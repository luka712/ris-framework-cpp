#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "camera.hpp"
#include "content.hpp"
#include "core.hpp"
#include "data.hpp"
#include "input.hpp"
#include "lights.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "scene.hpp"
#include "sprite_batch.hpp"
#include "ris/time.hpp"
#include "ris/utilities/listener_id_generator.hpp"
#include "ris/utilities/listener_map.hpp"
#include "window.hpp"

namespace ris {

enum class TextureFilteringPreset : uint32_t {
    Nearest = 0,
    Bilinear = 1,   ///< linear, no mip interpolation
    Trilinear = 2   ///< linear with mip interpolation
};

/// Mirrors `FrameworkConfiguration` of ris-framework-ts, with the desktop specific options.
struct FrameworkConfiguration {
    std::string title = "ris-framework";
    Vec2 back_buffer_size{960, 540};   ///< initial window size in pixels
    TextureFilteringPreset texture_filtering = TextureFilteringPreset::Trilinear;
    AngleBackend backend = AngleBackend::Auto;
    bool vsync = true;
    bool resizable = true;
    bool high_dpi = true;
    bool hidden = false;               ///< hidden window, for automated runs
    bool exit_on_escape = true;
    int32_t max_frames = -1;           ///< >= 0: stop after this many frames and use a deterministic 60 Hz time step
    double target_frames_per_second = 0; ///< > 0: `TimeManager::frame_end` sleeps to cap the frame rate; 0 = no cap
    double fixed_time_step_ms = 0;     ///< > 0: enables fixed update steps (`addOnFixedUpdateListener`) of this length
    std::string screenshot_path;       ///< when set, the last frame is written there as a binary PPM
};

/// Window access, like `IWindowManager`.
class RIS_API WindowManager {
public:
    explicit WindowManager(Window& window) : window_(window) {}
    Window& window() { return window_; }
    /// Drawable size in pixels.
    Vec2 size() const;
    void set_title(const std::string& title) { window_.set_title(title); }
    void set_fullscreen(bool enabled) { window_.set_fullscreen(enabled); }
    void quit() { window_.request_quit(); }

private:
    Window& window_;
};

/// Frame level rendering state, like `IRenderer`.
class RIS_API Renderer {
public:
    explicit Renderer(Window& window) : window_(window) {}

    Color clear_color = Color::black();

    /// Drawable size in pixels.
    Vec2 back_buffer_size() const;
    const char* backend_name() const { return window_.gl_info(GlInfo::AngleBackend); }
    const char* device_name() const { return window_.gl_info(GlInfo::Renderer); }

    /// Registers a listener that is called with the new drawable size when the window is resized.
    /// @return the id for `remove_on_resized_listener`. Ids come from a static `ListenerIdGenerator`.
    ListenerId add_on_resized_listener(std::function<void(Vec2)> listener) { return resized_.add(std::move(listener)); }
    /// Removes a resized listener. @return false for an unknown id.
    bool remove_on_resized_listener(ListenerId id) { return resized_.remove(id); }

private:
    friend class AFramework;
    static ListenerIdGenerator s_resizedIds;   ///< shared by all renderers
    Window& window_;
    ListenerMap<void(Vec2)> resized_{s_resizedIds};
};

class AFramework;

namespace detail {

/// Turns a user callback into the stored form `void(AFramework&, Args...)`. The callback may take the framework as
/// its first argument, or leave it out (like the listeners of ris-framework-ts, which capture the framework).
template <class... Args, class F>
std::function<void(AFramework&, Args...)> adaptListener(F callback) {
    if constexpr (std::is_invocable_v<F&, AFramework&, Args...>) {
        return std::function<void(AFramework&, Args...)>(std::move(callback));
    } else {
        static_assert(std::is_invocable_v<F&, Args...>,
                      "a framework listener must be callable as (AFramework&, args...) or as (args...)");
        return [callback = std::move(callback)](AFramework&, Args... args) mutable { callback(args...); };
    }
}

} // namespace detail

/// The base of the entry point, like `Framework` of ris-framework-ts. It owns the window, renderer, input, time and
/// the factories, runs the game loop and calls the listeners. Create a `ris::Framework`, not an `AFramework`.
///
///     ris::Framework framework(config);
///     framework.addOnLoadContentListener([&] { ... });
///     framework.addOnUpdateListener([&](const ris::GameTime& t) { ... });
///     framework.addOnRenderListener([&](ris::AFramework& f) { scene.render(); });   // both forms work
///     framework.initialize();   // runs the loop until the window closes
///
/// Listeners: every `add...Listener` returns a `ListenerId` for the matching `remove...Listener`. A callback is
/// called with the framework as its first argument and may leave that parameter out. Callbacks of one kind are
/// stored in a `std::map` keyed by id, ids come from a static `ListenerIdGenerator` per kind, and the callbacks
/// run in id (registration) order. Adding or removing listeners from inside a listener is safe (see
/// `ListenerMap`): a new listener first runs on the next dispatch, a removed one does not run any more.
///
/// Everything created from the framework (scenes, meshes, textures, materials) must be destroyed
/// before it. Not thread safe: use it from the thread that created it.
class RIS_API AFramework {
public:
    /// A listener without data: load content, initialized and render.
    using Callback = std::function<void(AFramework&)>;
    /// A listener that gets the time of the frame: update and fixed update.
    using UpdateCallback = std::function<void(AFramework&, const GameTime&)>;

    virtual ~AFramework();
    AFramework(const AFramework&) = delete;
    AFramework& operator=(const AFramework&) = delete;

    WindowManager& windowManager() { return *window_manager_; }
    Renderer& renderer() { return *renderer_; }
    /// The input manager (SDL3), created and initialised with the window.
    InputManager& inputManager() { return input_; }
    /// The time manager (SDL3 time API), created with the framework.
    TimeManager& timeManager() { return time_; }
    ContentManager& content() { return content_; }
    CameraFactory& cameraFactory() { return camera_factory_; }
    MaterialFactory& materialFactory() { return material_factory_; }
    MeshFactory& meshFactory() { return mesh_factory_; }
    SpriteBatch& spriteBatch() const { return *sprite_batch_; }
    const FrameworkConfiguration& configuration() const { return config_; }

    /// Registers a listener that runs once at the start of `initialize()`, before the initialized listeners, to
    /// load content. @param listener `void(AFramework&)` or `void()`. @return the id for `removeOnLoadContentListener`.
    template <class F>
    ListenerId addOnLoadContentListener(F listener) { return load_content_.add(detail::adaptListener<>(std::move(listener))); }
    /// Registers a listener that runs once in `initialize()`, after the load content listeners and before the loop.
    /// @param listener `void(AFramework&)` or `void()`. @return the id for `removeOnInitializedListener`.
    template <class F>
    ListenerId addOnInitializedListener(F listener) { return initialized_.add(detail::adaptListener<>(std::move(listener))); }
    /// Registers a listener that runs every frame after the input is updated.
    /// @param listener `void(AFramework&, const GameTime&)`, `void(const GameTime&)`.
    /// @return the id for `removeOnUpdateListener`.
    template <class F>
    ListenerId addOnUpdateListener(F listener) { return update_.add(detail::adaptListener<const GameTime&>(std::move(listener))); }
    /// Registers a listener for the fixed update steps: when `TimeManager::fixed_time_step_ms()` is > 0 (see
    /// `FrameworkConfiguration::fixed_time_step_ms`) it runs zero or more times per frame, before the update
    /// listeners, once for every full fixed step; its `GameTime::delta_time_ms` is the step and `elapsed_time_ms`
    /// the fixed simulation time. Never runs without a fixed time step.
    /// @param listener `void(AFramework&, const GameTime&)`, `void(const GameTime&)`.
    /// @return the id for `removeOnFixedUpdateListener`.
    template <class F>
    ListenerId addOnFixedUpdateListener(F listener) { return fixed_update_.add(detail::adaptListener<const GameTime&>(std::move(listener))); }
    /// Registers a listener that runs every frame after the screen is cleared, to draw.
    /// @param listener `void(AFramework&)` or `void()`. @return the id for `removeOnRenderListener`.
    template <class F>
    ListenerId addOnRenderListener(F listener) { return render_.add(detail::adaptListener<>(std::move(listener))); }

    /// Removes a load content listener. @return false for an unknown or already removed id.
    bool removeOnLoadContentListener(ListenerId id) { return load_content_.remove(id); }
    /// Removes an initialized listener. @return false for an unknown or already removed id.
    bool removeOnInitializedListener(ListenerId id) { return initialized_.remove(id); }
    /// Removes an update listener. @return false for an unknown or already removed id.
    bool removeOnUpdateListener(ListenerId id) { return update_.remove(id); }
    /// Removes a fixed update listener. @return false for an unknown or already removed id.
    bool removeOnFixedUpdateListener(ListenerId id) { return fixed_update_.remove(id); }
    /// Removes a render listener. @return false for an unknown or already removed id.
    bool removeOnRenderListener(ListenerId id) { return render_.remove(id); }

    /// Runs the load content and initialized listeners, then the game loop: events, input, update
    /// listeners, clear, render listeners, swap. Returns when the window is closed (or `max_frames` is reached).
    void initialize();
    /// Asks the loop to stop after the current frame.
    void quit() { quit_ = true; }

protected:
    /// Creates the window, renderer, input and the other services. Only derived classes (`Framework`) create it.
    explicit AFramework(const FrameworkConfiguration& config);

private:
    FrameworkConfiguration config_;
    std::unique_ptr<Window> window_;
    std::unique_ptr<WindowManager> window_manager_;
    std::unique_ptr<Renderer> renderer_;
    InputManager input_;   // declared after window_: disposed before the window goes away
    TimeManager time_;
    ContentManager content_;
    CameraFactory camera_factory_;
    MaterialFactory material_factory_;
    MeshFactory mesh_factory_;
    std::unique_ptr<SpriteBatch> sprite_batch_;

    // One static id generator per kind of listener, shared by all frameworks: ids are unique and increasing.
    static ListenerIdGenerator s_loadContentIds, s_initializedIds, s_updateIds, s_fixedUpdateIds, s_renderIds;

    ListenerMap<void(AFramework&)> load_content_{s_loadContentIds};
    ListenerMap<void(AFramework&)> initialized_{s_initializedIds};
    ListenerMap<void(AFramework&)> render_{s_renderIds};
    ListenerMap<void(AFramework&, const GameTime&)> update_{s_updateIds};
    ListenerMap<void(AFramework&, const GameTime&)> fixed_update_{s_fixedUpdateIds};
    bool quit_ = false;
};

/// The framework for the rendering API this library is built for (OpenGL ES through ANGLE / EGL).
/// Everything is documented on `AFramework`.
class RIS_API Framework : public AFramework {
public:
    explicit Framework(const FrameworkConfiguration& config = {}) : AFramework(config) {}
};

} // namespace ris
