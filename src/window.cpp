#include "ris/window.hpp"

#include <SDL3/SDL.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglext_angle.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

#include "internal.hpp"
#include "input/event_tap.hpp"

using namespace ris::detail;

namespace ris {

namespace {

std::atomic<bool> g_window_exists{false};

const char* egl_error_string(EGLint error) {
    switch (error) {
        case EGL_SUCCESS: return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED: return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS: return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC: return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE: return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONFIG: return "EGL_BAD_CONFIG";
        case EGL_BAD_CONTEXT: return "EGL_BAD_CONTEXT";
        case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY: return "EGL_BAD_DISPLAY";
        case EGL_BAD_MATCH: return "EGL_BAD_MATCH";
        case EGL_BAD_NATIVE_PIXMAP: return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
        case EGL_BAD_PARAMETER: return "EGL_BAD_PARAMETER";
        case EGL_BAD_SURFACE: return "EGL_BAD_SURFACE";
        case EGL_CONTEXT_LOST: return "EGL_CONTEXT_LOST";
        default: return "EGL_UNKNOWN";
    }
}

[[noreturn]] void throw_egl(const char* what) {
    const EGLint error = eglGetError();
    char hex[16];
    std::snprintf(hex, sizeof hex, "%04X", static_cast<unsigned>(error));
    throw std::runtime_error(std::string(what) + " failed: " + egl_error_string(error) + " (0x" + hex + ")");
}

struct BackendChoice {
    EGLint angle_type;
    const char* name;
};

AngleBackend backend_from_env() {
    const char* env = std::getenv("RIS_ANGLE_BACKEND");
    if (!env) {
        return AngleBackend::Auto;
    }
    std::string v;
    for (const char* p = env; *p; ++p) {
        v.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(*p))));
    }
    if (v == "opengl" || v == "gl") return AngleBackend::OpenGL;
    if (v == "opengles" || v == "gles") return AngleBackend::OpenGLES;
    if (v == "vulkan") return AngleBackend::Vulkan;
    if (v == "d3d11") return AngleBackend::D3D11;
    if (v == "metal") return AngleBackend::Metal;
    if (v == "null") return AngleBackend::Null;
    RIS_LOG_W("ignoring unknown RIS_ANGLE_BACKEND '%s'", env);
    return AngleBackend::Auto;
}

BackendChoice resolve_backend(AngleBackend requested) {
    if (requested == AngleBackend::Auto) {
        requested = backend_from_env();
    }
    if (requested == AngleBackend::Auto) {
#if defined(_WIN32)
        requested = AngleBackend::D3D11;
#elif defined(__APPLE__)
        requested = AngleBackend::Metal;
#else
        requested = AngleBackend::OpenGL;
#endif
    }
    switch (requested) {
        case AngleBackend::OpenGL: return {EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE, "OpenGL"};
        case AngleBackend::OpenGLES: return {EGL_PLATFORM_ANGLE_TYPE_OPENGLES_ANGLE, "OpenGLES"};
        case AngleBackend::Vulkan: return {EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE, "Vulkan"};
        case AngleBackend::D3D11: return {EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE, "D3D11"};
        case AngleBackend::Metal: return {EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE, "Metal"};
        case AngleBackend::Null: return {EGL_PLATFORM_ANGLE_TYPE_NULL_ANGLE, "Null"};
        default: return {EGL_PLATFORM_ANGLE_TYPE_DEFAULT_ANGLE, "Default"};
    }
}

void GL_APIENTRY gl_debug_callback(GLenum, GLenum type, GLuint, GLenum severity, GLsizei, const GLchar* message,
                                   const void*) {
    const LogLevel level = (type == GL_DEBUG_TYPE_ERROR_KHR || severity == GL_DEBUG_SEVERITY_HIGH_KHR)
                               ? LogLevel::Error
                               : (severity == GL_DEBUG_SEVERITY_NOTIFICATION_KHR ? LogLevel::Trace : LogLevel::Warn);
    log(level, "[gl] %s", message);
}

} // namespace

struct Window::Impl {
    SDL_Window* window = nullptr;
    bool sdl_initialised = false;
    void* metal_view = nullptr;

    EGLDisplay display = EGL_NO_DISPLAY;
    EGLConfig config = nullptr;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;

    std::string backend_name;
    std::string vendor, renderer, version, glsl_version, extensions;

    explicit Impl(const WindowDesc& desc);
    ~Impl();

    void create_window(const WindowDesc& desc);
    void create_egl(const WindowDesc& desc);
    EGLNativeWindowType native_window(EGLNativeDisplayType* native_display_out);
};

Window::Impl::Impl(const WindowDesc& desc) {
    create_window(desc);
    create_egl(desc);
}

Window::Impl::~Impl() {
    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
        if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
        eglTerminate(display);
    }
#if defined(__APPLE__)
    if (metal_view) SDL_Metal_DestroyView(static_cast<SDL_MetalView>(metal_view));
#endif
    if (window) SDL_DestroyWindow(window);
    if (sdl_initialised) SDL_Quit();
}

void Window::Impl::create_window(const WindowDesc& desc) {
#if defined(__linux__) && !defined(__ANDROID__)
    // ANGLE's OpenGL backend renders to X11 windows. Prefer X11 (XWayland on Wayland sessions)
    // unless the user chose a video driver explicitly.
    if (!std::getenv("SDL_VIDEO_DRIVER")) {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
    }
#endif
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }
    sdl_initialised = true;

    SDL_WindowFlags flags = 0;
    if (has_flag(desc.flags, WindowFlags::Resizable)) flags |= SDL_WINDOW_RESIZABLE;
    if (has_flag(desc.flags, WindowFlags::Fullscreen)) flags |= SDL_WINDOW_FULLSCREEN;
    if (has_flag(desc.flags, WindowFlags::Borderless)) flags |= SDL_WINDOW_BORDERLESS;
    if (has_flag(desc.flags, WindowFlags::HighDpi)) flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (has_flag(desc.flags, WindowFlags::Hidden)) flags |= SDL_WINDOW_HIDDEN;
    if (has_flag(desc.flags, WindowFlags::Maximized)) flags |= SDL_WINDOW_MAXIMIZED;
#if defined(__APPLE__)
    flags |= SDL_WINDOW_METAL;
#endif

    window = SDL_CreateWindow(desc.title.c_str(), desc.width, desc.height, flags);
    if (!window) {
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }
    RIS_LOG_I("SDL video driver: %s", SDL_GetCurrentVideoDriver());
}

EGLNativeWindowType Window::Impl::native_window(EGLNativeDisplayType* native_display_out) {
    const SDL_PropertiesID props = SDL_GetWindowProperties(window);
    *native_display_out = EGL_DEFAULT_DISPLAY;

#if defined(_WIN32)
    void* hwnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    if (!hwnd) throw std::runtime_error("could not get the Win32 window handle");
    return reinterpret_cast<EGLNativeWindowType>(hwnd);
#elif defined(__APPLE__)
    metal_view = SDL_Metal_CreateView(window);
    if (!metal_view) throw std::runtime_error(std::string("SDL_Metal_CreateView failed: ") + SDL_GetError());
    void* layer = SDL_Metal_GetLayer(static_cast<SDL_MetalView>(metal_view));
    if (!layer) throw std::runtime_error(std::string("SDL_Metal_GetLayer failed: ") + SDL_GetError());
    return reinterpret_cast<EGLNativeWindowType>(layer);
#elif defined(__ANDROID__)
    void* native = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);
    if (!native) throw std::runtime_error("could not get the ANativeWindow");
    return reinterpret_cast<EGLNativeWindowType>(native);
#else
    const char* driver = SDL_GetCurrentVideoDriver();
    if (!driver || std::strcmp(driver, "x11") != 0) {
        throw std::domain_error(std::string("ANGLE needs the X11 video driver on Linux (current: ")
                                + (driver ? driver : "none") +
                                 "). Run with SDL_VIDEO_DRIVER=x11.");
    }
    void* x_display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
    const Sint64 x_window = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    if (!x_display || x_window == 0) throw std::runtime_error("could not get the X11 window handle");
    *native_display_out = reinterpret_cast<EGLNativeDisplayType>(x_display);
    return static_cast<EGLNativeWindowType>(static_cast<uintptr_t>(x_window));
#endif
}

void Window::Impl::create_egl(const WindowDesc& desc) {
    EGLNativeDisplayType native_display = EGL_DEFAULT_DISPLAY;
    const EGLNativeWindowType native = native_window(&native_display);

    const BackendChoice backend = resolve_backend(desc.backend);

    // The EGL implementation we are linked against is ANGLE on Windows and macOS (vcpkg). The vcpkg
    // ANGLE port ships no rendering backend on Linux, so there the system EGL (Mesa, vendor drivers)
    // is used, and a real ANGLE libEGL is only picked up when it is the one loaded at runtime.
    const char* client_extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    const bool is_angle = client_extensions && std::strstr(client_extensions, "EGL_ANGLE_platform_angle") != nullptr;
    if (is_angle) {
        backend_name = backend.name;
        const EGLAttrib display_attribs[] = {EGL_PLATFORM_ANGLE_TYPE_ANGLE, backend.angle_type, EGL_NONE};
        display = eglGetPlatformDisplay(EGL_PLATFORM_ANGLE_ANGLE, reinterpret_cast<void*>(native_display), display_attribs);
        if (display == EGL_NO_DISPLAY) throw_egl("eglGetPlatformDisplay(ANGLE)");
    } else {
        backend_name = "native EGL";
        RIS_LOG_W("the loaded EGL library is not ANGLE, using the native EGL implementation");
#if defined(__linux__) && !defined(__ANDROID__)
        if (client_extensions && std::strstr(client_extensions, "EGL_KHR_platform_x11")) {
            constexpr EGLenum platform_x11 = 0x31D5; // EGL_PLATFORM_X11_KHR
            display = eglGetPlatformDisplay(platform_x11, reinterpret_cast<void*>(native_display), nullptr);
        }
#endif
        if (display == EGL_NO_DISPLAY) display = eglGetDisplay(native_display);
        if (display == EGL_NO_DISPLAY) throw_egl("eglGetDisplay");
    }

    EGLint egl_major = 0, egl_minor = 0;
    if (!eglInitialize(display, &egl_major, &egl_minor)) throw_egl("eglInitialize");
    RIS_LOG_I("EGL %d.%d, %s%s", egl_major, egl_minor, is_angle ? "ANGLE backend " : "", backend_name.c_str());

    if (!eglBindAPI(EGL_OPENGL_ES_API)) throw_egl("eglBindAPI");

    auto choose_config = [&](uint32_t samples) {
        const EGLint attribs[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                                  EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                                  EGL_RED_SIZE, 8,
                                  EGL_GREEN_SIZE, 8,
                                  EGL_BLUE_SIZE, 8,
                                  EGL_ALPHA_SIZE, 8,
                                  EGL_DEPTH_SIZE, static_cast<EGLint>(desc.depth_bits),
                                  EGL_STENCIL_SIZE, static_cast<EGLint>(desc.stencil_bits),
                                  EGL_SAMPLE_BUFFERS, samples > 0 ? 1 : 0,
                                  EGL_SAMPLES, static_cast<EGLint>(samples),
                                  EGL_NONE};
        EGLint count = 0;
        return eglChooseConfig(display, attribs, &config, 1, &count) && count > 0;
    };
    if (!choose_config(desc.msaa_samples)) {
        if (desc.msaa_samples == 0 || !choose_config(0)) {
            throw std::runtime_error(std::string("no matching EGL config (ES3, RGBA8, depth ")
                                     + std::to_string(desc.depth_bits) + ", stencil "
                                     + std::to_string(desc.stencil_bits) + ", " + std::to_string(desc.msaa_samples)
                                     + " samples)");
        }
        RIS_LOG_W("%u x MSAA is not available, continuing without", desc.msaa_samples);
    }

    const EGLint context_attribs[] = {EGL_CONTEXT_MAJOR_VERSION, 3,
                                      EGL_CONTEXT_MINOR_VERSION, 0,
                                      EGL_CONTEXT_OPENGL_DEBUG, desc.debug ? EGL_TRUE : EGL_FALSE,
                                      EGL_NONE};
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribs);
    if (context == EGL_NO_CONTEXT) throw_egl("eglCreateContext(ES 3.0)");

    surface = eglCreateWindowSurface(display, config, native, nullptr);
    if (surface == EGL_NO_SURFACE) throw_egl("eglCreateWindowSurface");

    if (!eglMakeCurrent(display, surface, surface, context)) throw_egl("eglMakeCurrent");
    eglSwapInterval(display, desc.vsync ? 1 : 0);

    auto gl_string = [](GLenum name) {
        const GLubyte* s = glGetString(name);
        return std::string(s ? reinterpret_cast<const char*>(s) : "");
    };
    vendor = gl_string(GL_VENDOR);
    renderer = gl_string(GL_RENDERER);
    version = gl_string(GL_VERSION);
    glsl_version = gl_string(GL_SHADING_LANGUAGE_VERSION);
    extensions = gl_string(GL_EXTENSIONS);
    RIS_LOG_I("GL_RENDERER: %s", renderer.c_str());
    RIS_LOG_I("GL_VERSION:  %s", version.c_str());

    if (desc.debug && extensions.find("GL_KHR_debug") != std::string::npos) {
        auto callback_fn = reinterpret_cast<PFNGLDEBUGMESSAGECALLBACKKHRPROC>(eglGetProcAddress("glDebugMessageCallbackKHR"));
        if (callback_fn) {
            glEnable(GL_DEBUG_OUTPUT_KHR);
            callback_fn(gl_debug_callback, nullptr);
        }
    }
}

// ---- Window ---------------------------------------------------------------------------------

Window::Window(const WindowDesc& desc) {
    if (desc.width <= 0 || desc.height <= 0) {
        throw std::invalid_argument(std::string("window size must be positive (got ") + std::to_string(desc.width)
                                    + "x" + std::to_string(desc.height) + ")");
    }
    bool expected = false;
    if (!g_window_exists.compare_exchange_strong(expected, true)) {
        throw std::domain_error("only one ris::Window can exist at a time");
    }
    try {
        impl_ = std::make_unique<Impl>(desc);
    } catch (...) {
        g_window_exists.store(false);
        throw;
    }
    set_context_alive(true);
}

Window::~Window() {
    if (impl_) {
        set_context_alive(false);
        impl_.reset();
        g_window_exists.store(false);
    }
}

Window::Window(Window&&) noexcept = default;

Window& Window::operator=(Window&& other) noexcept {
    if (this != &other) {
        if (impl_) {
            set_context_alive(false);
            impl_.reset();
            g_window_exists.store(false);
        }
        impl_ = std::move(other.impl_);
    }
    return *this;
}

bool Window::poll_event(Event& out) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        // The input manager sees every event, including the ones that have no `Event` (touch, gamepads).
        detail::dispatch_event_tap(e);
        Event ev;
        ev.timestamp_ns = e.common.timestamp;
        switch (e.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                ev.type = EventType::Quit;
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                ev.type = EventType::WindowResized;
                ev.width = e.window.data1;
                ev.height = e.window.data2;
                break;
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                ev.type = EventType::WindowFocusGained;
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                ev.type = EventType::WindowFocusLost;
                break;
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: {
                ev.type = e.type == SDL_EVENT_KEY_DOWN ? EventType::KeyDown : EventType::KeyUp;
                ev.scancode = static_cast<int32_t>(e.key.scancode);
                ev.keycode = static_cast<int32_t>(e.key.key);
                ev.repeat = e.key.repeat ? 1u : 0u;
                if (e.key.mod & SDL_KMOD_SHIFT) ev.mods |= key_mod::shift;
                if (e.key.mod & SDL_KMOD_CTRL) ev.mods |= key_mod::ctrl;
                if (e.key.mod & SDL_KMOD_ALT) ev.mods |= key_mod::alt;
                if (e.key.mod & SDL_KMOD_GUI) ev.mods |= key_mod::gui;
                break;
            }
            case SDL_EVENT_TEXT_INPUT:
                ev.type = EventType::TextInput;
                std::strncpy(ev.text, e.text.text, sizeof(ev.text) - 1);
                break;
            case SDL_EVENT_MOUSE_MOTION:
                ev.type = EventType::MouseMove;
                ev.x = e.motion.x;
                ev.y = e.motion.y;
                ev.dx = e.motion.xrel;
                ev.dy = e.motion.yrel;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                ev.type = e.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? EventType::MouseButtonDown : EventType::MouseButtonUp;
                ev.button = e.button.button;
                ev.clicks = e.button.clicks;
                ev.x = e.button.x;
                ev.y = e.button.y;
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                ev.type = EventType::MouseWheel;
                ev.dx = e.wheel.x;
                ev.dy = e.wheel.y;
                break;
            default:
                continue;   // an event we do not expose
        }
        out = ev;
        return true;
    }
    return false;
}

void Window::swap_buffers() {
    if (!eglSwapBuffers(impl_->display, impl_->surface)) {
        throw_egl("eglSwapBuffers");
    }
}

void Window::set_vsync(bool enabled) { eglSwapInterval(impl_->display, enabled ? 1 : 0); }
void Window::set_title(const std::string& title) { SDL_SetWindowTitle(impl_->window, title.c_str()); }
void Window::set_fullscreen(bool enabled) { SDL_SetWindowFullscreen(impl_->window, enabled); }
void Window::show() { SDL_ShowWindow(impl_->window); }

void Window::set_text_input(bool enabled) {
    if (enabled) {
        SDL_StartTextInput(impl_->window);
    } else {
        SDL_StopTextInput(impl_->window);
    }
}

void Window::request_quit() {
    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&e);
}

void Window::size(int32_t& width, int32_t& height) const {
    int w = 0, h = 0;
    SDL_GetWindowSize(impl_->window, &w, &h);
    width = w;
    height = h;
}

void Window::drawable_size(int32_t& width, int32_t& height) const {
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(impl_->window, &w, &h);
    width = w;
    height = h;
}

float Window::content_scale() const {
    const float scale = SDL_GetWindowDisplayScale(impl_->window);
    return scale > 0.0f ? scale : 1.0f;
}

const char* Window::gl_info(GlInfo which) const {
    switch (which) {
        case GlInfo::Vendor: return impl_->vendor.c_str();
        case GlInfo::Renderer: return impl_->renderer.c_str();
        case GlInfo::Version: return impl_->version.c_str();
        case GlInfo::GlslVersion: return impl_->glsl_version.c_str();
        case GlInfo::Extensions: return impl_->extensions.c_str();
        case GlInfo::AngleBackend: return impl_->backend_name.c_str();
    }
    return "";
}

bool Window::has_gl_extension(const char* name) const {
    if (!name || !*name) {
        return false;
    }
    const std::string& list = impl_->extensions;
    const size_t len = std::strlen(name);
    size_t pos = 0;
    while ((pos = list.find(name, pos)) != std::string::npos) {
        const bool start_ok = pos == 0 || list[pos - 1] == ' ';
        const bool end_ok = pos + len == list.size() || list[pos + len] == ' ';
        if (start_ok && end_ok) {
            return true;
        }
        pos += len;
    }
    return false;
}

namespace input {

bool key_down(int32_t scancode) {
    int count = 0;
    const bool* state = SDL_GetKeyboardState(&count);
    return state && scancode >= 0 && scancode < count && state[scancode];
}

uint32_t mouse_state(float* x, float* y) {
    float mx = 0, my = 0;
    const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
    if (x) *x = mx;
    if (y) *y = my;
    return static_cast<uint32_t>(buttons);
}

} // namespace input

} // namespace ris
