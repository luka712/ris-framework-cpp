#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <SDL3/SDL_video.h>

#include "ris/core.hpp"
#include "ris/math.hpp"

union SDL_Event;

namespace ris {

/// The descriptor for created window.
struct WindowDescriptor {
    std::string title = "ris-framework";
    int32_t width = 1280;
    int32_t height = 720;
};

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



/// The window manager of the SDL3 window. `initializeForGles()` initialises SDL, creates the SDL3 window and the
/// EGL display (ANGLE on Windows and macOS) and makes the OpenGL ES 3 context current on the calling thread; the
/// destructor tears everything down. All GPU objects (Buffer, Texture, ...) must be destroyed before the window manager.
///
/// It owns the window and runs its event loop, nothing else: keyboard, mouse, gamepad and touch input are handled by
/// the `InputManager`, which gets the SDL events through `addOnSdlEventListener`.
///
/// Only one window can exist at a time in 0.1.0, and it must be used from the thread that created it.
/// Throws standard exceptions (std::runtime_error, ...) on failure.
class RIS_API WindowManager {
public:
    /// The window is created by `initializeForGles()`.
    explicit WindowManager(const WindowDescriptor& desc = {});
    ~WindowManager();
    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;

    glm::vec2 getWindowBounds() const;
    const std::string& getTitle() const { return m_windowDesc.title; }
    void setTitle(const std::string& title);
    void setFullscreen(bool enabled);
    void setVsync(bool enabled);
    void swapBuffers();

    uint32_t addOnUpdateListener(std::function<void()> listener) {
        s_updateIds++;
        onUpdateListeners_[s_updateIds] = std::move(listener);
        return s_updateIds;
    }
    uint32_t addOnRenderListener(std::function<void()> listener) {
        s_renderIds++;
        onRenderListeners_[s_renderIds] = std::move(listener);
        return s_renderIds;
    }
    uint32_t addOnResizeListener(std::function<void(WindowManager&, glm::vec2)> listener) {
        s_resizeIds++;
        onResizeListeners_[s_resizeIds] = std::move(listener);
        return s_resizeIds;
    }
    bool removeOnUpdateListener(uint32_t id) { return onUpdateListeners_.erase(id) > 0; }
    bool removeOnRenderListener(uint32_t id) { return onRenderListeners_.erase(id) > 0; }
    bool removeOnResizeListener(uint32_t id) { return onResizeListeners_.erase(id) > 0; }

    void initializeForGles();
    void runEventLoop();
    void quit() { quit_ = true; }

    /// Registers a listener that is called with every SDL event the event loop pulls from SDL, before the loop
    /// handles it. This is how the `InputManager` gets its events. @return its id, for `removeOnSdlEventListener`.
    uint32_t addOnSdlEventListener(std::function<void(const SDL_Event&)> listener) {
        s_eventIds++;
        onEventListeners_[s_eventIds] = std::move(listener);
        return s_eventIds;
    }
    bool removeOnSdlEventListener(uint32_t id) { return onEventListeners_.erase(id) > 0; }

    /// The SDL window, null before `initializeForGles()`.
    SDL_Window* getWindow() const { return m_window; }
    /// The native API ANGLE translates to ("OpenGL", "D3D11", "Metal", ...) or "native EGL"; empty before `initializeForGles()`.
    const std::string& getAngleBackend() const { return angleBackend_; }

private:
    void release() noexcept;
    void requireWindow() const;

    // The last id given, one counter per kind of listener, shared by all window managers (ids start at 1, 0 is never used).
    static uint32_t s_updateIds, s_renderIds, s_resizeIds, s_eventIds;

    WindowDescriptor m_windowDesc;
    SDL_Window* m_window = nullptr;
    bool m_sdlInitialized = false;
    SDL_WindowFlags m_windowFlags = SDL_WINDOW_RESIZABLE;

    std::string angleBackend_;
    std::map<uint32_t, std::function<void()> > onUpdateListeners_, onRenderListeners_;
    std::map<uint32_t, std::function<void(WindowManager&, glm::vec2)> > onResizeListeners_;
    std::map<uint32_t, std::function<void(const SDL_Event&)> > onEventListeners_;
    bool quit_ = false;
};

} // namespace ris
