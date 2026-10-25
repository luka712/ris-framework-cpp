#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "camera.hpp"
#include "content.hpp"
#include "core.hpp"
#include "data.hpp"
#include "input/input.hpp"
#include "lights.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "scene.hpp"
#include "sprite_batch.hpp"
#include "time.hpp"
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
    int32_t max_frames = -1;           ///< >= 0: stop after this many frames and use a fixed 60 Hz time step
    std::string screenshot_path;       ///< when set, the last frame is written there as a binary PPM
};

using ListenerId = uint32_t;

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

    ListenerId add_on_resized_listener(std::function<void(Vec2)> listener);
    bool remove_on_resized_listener(ListenerId id);

private:
    friend class Framework;
    Window& window_;
    std::vector<std::pair<ListenerId, std::function<void(Vec2)>>> resized_;
    ListenerId next_id_ = 1;
};

/// The entry point, like `Framework` of ris-framework-ts.
///
///     ris::Framework framework(config);
///     framework.add_on_load_content_listener([&] { ... });
///     framework.add_on_update_listener([&](const ris::GameTime& t) { ... });
///     framework.add_on_render_listener([&] { scene.render(); });
///     framework.initialize();   // runs the loop until the window closes
///
/// Everything created from the framework (scenes, meshes, textures, materials) must be destroyed
/// before it. Not thread safe: use it from the thread that created it.
class RIS_API Framework {
public:
    explicit Framework(const FrameworkConfiguration& config = {});
    ~Framework();
    Framework(const Framework&) = delete;
    Framework& operator=(const Framework&) = delete;

    WindowManager& window_manager() { return *window_manager_; }
    Renderer& renderer() { return *renderer_; }
    /// The input manager of the default backend (SDL3), created and initialised with the window.
    AInputManager& input_manager() { return *input_; }
    TimeManager& time_manager() { return time_; }
    ContentManager& content() { return content_; }
    CameraFactory& camera_factory() { return camera_factory_; }
    MaterialFactory& material_factory() { return material_factory_; }
    MeshFactory& mesh_factory() { return mesh_factory_; }
    SpriteBatch& sprite_batch() { return *sprite_batch_; }
    const FrameworkConfiguration& configuration() const { return config_; }

    ListenerId add_on_load_content_listener(std::function<void()> listener);
    ListenerId add_on_initialized_listener(std::function<void()> listener);
    ListenerId add_on_update_listener(std::function<void(const GameTime&)> listener);
    ListenerId add_on_render_listener(std::function<void()> listener);
    bool remove_on_load_content_listener(ListenerId id);
    bool remove_on_initialized_listener(ListenerId id);
    bool remove_on_update_listener(ListenerId id);
    bool remove_on_render_listener(ListenerId id);

    /// Runs the load content and initialized listeners, then the game loop: events, input, update
    /// listeners, clear, render listeners, swap. Returns when the window is closed (or `max_frames` is reached).
    void initialize();
    /// Asks the loop to stop after the current frame.
    void quit() { quit_ = true; }

private:
    template <class F>
    using Listeners = std::vector<std::pair<ListenerId, F>>;

    FrameworkConfiguration config_;
    std::unique_ptr<Window> window_;
    std::unique_ptr<WindowManager> window_manager_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<AInputManager> input_;   // declared after window_: disposed before the window goes away
    TimeManager time_;
    ContentManager content_;
    CameraFactory camera_factory_;
    MaterialFactory material_factory_;
    MeshFactory mesh_factory_;
    std::unique_ptr<SpriteBatch> sprite_batch_;

    Listeners<std::function<void()>> load_content_, initialized_, render_;
    Listeners<std::function<void(const GameTime&)>> update_;
    ListenerId next_id_ = 1;
    bool quit_ = false;
};

} // namespace ris
