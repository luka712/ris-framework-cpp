#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "core.hpp"
#include "event.hpp"

namespace ris {

enum class WindowFlags : uint32_t {
    None = 0,
    Resizable = 1u << 0,
    Fullscreen = 1u << 1,
    Borderless = 1u << 2,
    HighDpi = 1u << 3,
    Hidden = 1u << 4,
    Maximized = 1u << 5
};

constexpr WindowFlags operator|(WindowFlags a, WindowFlags b) noexcept {
    return static_cast<WindowFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
constexpr bool has_flag(WindowFlags flags, WindowFlags f) noexcept {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(f)) != 0;
}

/// Which native API ANGLE translates OpenGL ES calls to.
enum class AngleBackend : uint32_t {
    Auto = 0,   ///< Windows: D3D11, macOS / iOS: Metal, Linux: OpenGL. `RIS_ANGLE_BACKEND` env var overrides.
    OpenGL = 1,
    OpenGLES = 2,
    Vulkan = 3,
    D3D11 = 4,
    Metal = 5,
    Null = 6    ///< no rendering, for headless tests
};

struct WindowDesc {
    std::string title = "ris-framework";
    int32_t width = 1280;
    int32_t height = 720;
    WindowFlags flags = WindowFlags::Resizable;
    AngleBackend backend = AngleBackend::Auto;
    bool vsync = true;
    uint32_t msaa_samples = 0;  ///< 0 = off, otherwise 2, 4, 8
    uint32_t depth_bits = 24;   ///< 0, 16, 24
    uint32_t stencil_bits = 8;  ///< 0 or 8
    bool debug = false;         ///< request a debug context and log GL debug messages
};

enum class GlInfo : uint32_t {
    Vendor = 0,
    Renderer = 1,
    Version = 2,
    GlslVersion = 3,
    Extensions = 4,
    AngleBackend = 5    ///< "OpenGL", "D3D11", "Metal", ...
};

/**
 * A window with an OpenGL ES 3 context running on ANGLE. Creating it initialises SDL, creates the
 * ANGLE EGL display and makes the context current on the calling thread. Destroying it tears
 * everything down; all GPU objects (Buffer, Texture, ...) must be destroyed before the Window.
 *
 * Only one Window can exist at a time in 0.1.0, and it must be used from the thread that created it.
 * Throws standard exceptions (std::runtime_error, ...) on failure.
 */
class RIS_API Window {
public:
    explicit Window(const WindowDesc& desc = {});
    ~Window();
    Window(Window&&) noexcept;
    Window& operator=(Window&&) noexcept;
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    /// Pumps the OS event queue. Returns true and fills `event` while events are pending.
    bool poll_event(Event& event);
    void swap_buffers();

    void set_vsync(bool enabled);
    void set_title(const std::string& title);
    void set_fullscreen(bool enabled);
    void show();
    void set_text_input(bool enabled);
    /// Queues an EventType::Quit event.
    void request_quit();

    /// Size in screen coordinates.
    void size(int32_t& width, int32_t& height) const;
    /// Size of the drawable in pixels (differs from size() on high DPI displays).
    void drawable_size(int32_t& width, int32_t& height) const;
    float content_scale() const;

    /// The returned string stays valid until the Window is destroyed.
    const char* gl_info(GlInfo which) const;
    bool has_gl_extension(const char* name) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

namespace input {
/// Polled state, valid after events were pumped with Window::poll_event.
RIS_API bool key_down(int32_t scancode);
/// Returns the button bit mask (bit 0 = left). Either pointer may be null.
RIS_API uint32_t mouse_state(float* x, float* y);
} // namespace input

} // namespace ris
