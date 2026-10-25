#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
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
#include "ris/rendering/a_graphics_device.hpp"
#include "ris/rendering/sampler/texture_filtering_preset.hpp"
#include "ris/rendering/texture/a_texture_factory.hpp"
#include "ris/time.hpp"
#include "window.hpp"

namespace ris {
    // TextureFilteringPreset (Nearest, Bilinear, Trilinear) is in ris/rendering/sampler/texture_filtering_preset.hpp

    /// Mirrors `FrameworkConfiguration` of ris-framework-ts, with the desktop specific options.
    struct FrameworkConfiguration {
        std::string title = "ris-framework";
        Vec2 backBufferSize{960, 540}; ///< initial window size in pixels
        TextureFilteringPreset textureFiltering = TextureFilteringPreset::Trilinear;
        AngleBackend backend = AngleBackend::Auto;
        bool vsync = true;
        bool resizable = true;
        bool high_dpi = true;
        bool hidden = false; ///< hidden window, for automated runs
        bool exit_on_escape = true;
        int32_t max_frames = -1; ///< >= 0: stop after this many frames and use a deterministic 60 Hz time step
        double target_frames_per_second = 0; ///< > 0: `TimeManager::frame_end` sleeps to cap the frame rate; 0 = no cap
        double fixed_time_step_ms = 0; ///< > 0: enables fixed update steps (`addOnFixedUpdateListener`) of this length
        std::string screenshot_path; ///< when set, the last frame is written there as a binary PPM
    };

    /// Window access, like `IWindowManager`.
    class RIS_API WindowManager {
    public:
        explicit WindowManager(Window &window) : window_(window) {
        }

        Window &window() { return window_; }

        /// Drawable size in pixels.
        Vec2 size() const;

        void set_title(const std::string &title) { window_.set_title(title); }
        void set_fullscreen(bool enabled) { window_.set_fullscreen(enabled); }
        void quit() { window_.request_quit(); }

    private:
        Window &window_;
    };

    /// Frame level rendering state, like `IRenderer`.
    class RIS_API Renderer {
    public:
        explicit Renderer(Window &window) : window_(window) {
        }

        /// The colour the back buffer is cleared with at the start of every frame. Black by default.
        Color getClearColor() const { return m_clearColor; }
        void setClearColor(Color color) { m_clearColor = color; }

        /// Drawable size in pixels.
        Vec2 back_buffer_size() const;

        const char *backend_name() const { return window_.gl_info(GlInfo::AngleBackend); }
        const char *device_name() const { return window_.gl_info(GlInfo::Renderer); }

        /// Registers a listener that is called with the new drawable size when the window is resized.
        /// @return its id (1, 2, 3, ...), for `remove_on_resized_listener`.
        uint32_t add_on_resized_listener(std::function<void(Vec2)> listener) {
            const uint32_t id = ++s_resizedIds;
            resized_[id] = std::move(listener);
            return id;
        }

        /// Removes a resized listener. @return false for an unknown id.
        bool remove_on_resized_listener(uint32_t id) { return resized_.erase(id) > 0; }

    private:
        friend class Framework;
        static uint32_t s_resizedIds; ///< shared by all renderers, the last id given
        Window &window_;
        Color m_clearColor = Color::black();
        std::map<uint32_t, std::function<void(Vec2)> > resized_;
    };

    class Framework;

    /// The entry point, like `Framework` of ris-framework-ts. It owns the window, renderer, input, time and
    /// the factories, runs the game loop and calls the listeners.
    ///
    ///     ris::Framework framework(config);
    ///     framework.addOnLoadContentListener([&] { ... });
    ///     framework.addOnUpdateListener([&](const ris::GameTime& t) { ... });
    ///     framework.addOnRenderListener([&](ris::Framework& f) { scene.render(); });   // both forms work
    ///     framework.initialize();   // runs the loop until the window closes
    ///
    /// Listeners: every `add...Listener` returns an id (1, 2, 3, ...) for the matching `remove...Listener`. The
    /// callbacks of one kind are kept in a `std::map` keyed by id, so they run in registration order. A callback may
    /// take the framework as its first argument or leave it out. Listeners may be added or removed from inside a
    /// listener: a new one first runs on the next dispatch, a removed one is not called any more.
    ///
    /// Everything created from the framework (scenes, meshes, textures, materials) must be destroyed
    /// before it. Not thread safe: use it from the thread that created it.
    class RIS_API Framework {
    public:
        /// Creates the window, renderer, input, time and the other services.
        explicit Framework(const FrameworkConfiguration &config);

        // TODO: doc comment
        virtual ~Framework();

        // TODO: doc comment
        Framework(const Framework &) = delete;

        WindowManager &windowManager() { return *window_manager_; }
        Renderer &renderer() { return *renderer_; }
        /// The input manager (SDL3), created and initialised with the window.
        InputManager &inputManager() { return input_; }
        /// The time manager (SDL3 time API), created with the framework.
        TimeManager &timeManager() { return time_; }
        ContentManager &content() { return content_; }
        CameraFactory &cameraFactory() { return camera_factory_; }
        MaterialFactory &materialFactory() { return material_factory_; }
        MeshFactory &meshFactory() { return mesh_factory_; }
        /// Creates textures with the rendering backend of the framework (OpenGL ES), see ATextureFactory.
        ATextureFactory &textureFactory() { return *texture_factory_; }
        /// The graphics device of the rendering backend of the framework (OpenGL ES): GPU info and features, default
        /// sampler / blend / primitive state, and the creation of samplers, render passes and swap chains. Created
        /// and initialised with the window, see AGraphicsDevice.
        AGraphicsDevice &graphicsDevice() { return *graphics_device_; }
        SpriteBatch &spriteBatch() const { return *sprite_batch_; }
        const FrameworkConfiguration &configuration() const { return config_; }

        /// Registers a listener that runs once at the start of `initialize()`, before the initialized listeners, to
        /// load content. @return its id, for `removeOnLoadContentListener`.
        uint32_t addOnLoadContentListener(std::function<void(Framework &)> listener) {
            s_loadContentIds++;
            m_onLoadContentListeners[s_loadContentIds] = std::move(listener);
            return s_loadContentIds;
        }

        /// Registers a listener that runs once in `initialize()`, after the load content listeners and before the loop.
        /// @return its id, for `removeOnInitializedListener`.
        uint32_t addOnInitializedListener(std::function<void(Framework &)> listener) {
            s_initializedIds++;
            m_onInitializedListeners[s_initializedIds] = std::move(listener);
            return s_initializedIds;
        }

        /// Registers a listener that runs every frame after the input is updated.
        /// @return its id, for `removeOnUpdateListener`.
        uint32_t addOnUpdateListener(std::function<void(Framework &, const GameTime &)> listener) {
            s_updateIds++;
            m_onUpdateListeners[s_updateIds] = std::move(listener);
            return s_updateIds;
        }

        /// Registers a listener for the fixed update steps: when `TimeManager::fixed_time_step_ms()` is > 0 (see
        /// `FrameworkConfiguration::fixed_time_step_ms`) it runs zero or more times per frame, before the update
        /// listeners, once for every full fixed step; its `GameTime::delta_time_ms` is the step and `elapsed_time_ms`
        /// the fixed simulation time. Never runs without a fixed time step. @return its id, for `removeOnFixedUpdateListener`.
        uint32_t addOnFixedUpdateListener(std::function<void(Framework &, const GameTime &)> listener) {
            s_fixedUpdateIds++;
            m_onFixedUpdateListeners[s_fixedUpdateIds] = std::move(listener);
            return s_fixedUpdateIds;
        }

        /// Registers a listener that runs every frame after the screen is cleared, to draw.
        /// @return its id, for `removeOnRenderListener`.
        uint32_t addOnRenderListener(std::function<void(Framework &)> listener) {
            s_renderIds++;
            m_onRenderListeners[s_renderIds] = std::move(listener);
            return s_renderIds;
        }

        /// The remove functions return false for an unknown or already removed id.
        bool removeOnLoadContentListener(uint32_t id) { return m_onLoadContentListeners.erase(id) > 0; }
        bool removeOnInitializedListener(uint32_t id) { return m_onInitializedListeners.erase(id) > 0; }
        bool removeOnUpdateListener(uint32_t id) { return m_onUpdateListeners.erase(id) > 0; }
        bool removeOnFixedUpdateListener(uint32_t id) { return m_onFixedUpdateListeners.erase(id) > 0; }
        bool removeOnRenderListener(uint32_t id) { return m_onRenderListeners.erase(id) > 0; }

        /// Runs the load content and initialized listeners, then the game loop: events, input, update
        /// listeners, clear, render listeners, swap. Returns when the window is closed (or `max_frames` is reached).
        void initialize();

        /// Asks the loop to stop after the current frame.
        void quit() { quit_ = true; }

    private:
        FrameworkConfiguration config_;
        std::unique_ptr<Window> window_;
        std::unique_ptr<WindowManager> window_manager_;
        std::unique_ptr<Renderer> renderer_;
        InputManager input_; // declared after window_: disposed before the window goes away
        TimeManager time_;
        ContentManager content_;
        CameraFactory camera_factory_;
        MaterialFactory material_factory_;
        MeshFactory mesh_factory_;
        std::unique_ptr<ATextureFactory> texture_factory_;
        std::unique_ptr<AGraphicsDevice> graphics_device_; // after window_: destroyed before the window goes away
        std::unique_ptr<SpriteBatch> sprite_batch_;

        // The last id given, one counter per kind of listener, shared by all frameworks (ids start at 1, 0 is never used).
        static uint32_t s_loadContentIds, s_initializedIds, s_updateIds, s_fixedUpdateIds, s_renderIds;

        std::map<uint32_t, std::function<void(Framework &)> > m_onLoadContentListeners, m_onInitializedListeners,
                m_onRenderListeners;
        std::map<uint32_t, std::function<void(Framework &, const GameTime &)> > m_onUpdateListeners,
                m_onFixedUpdateListeners;

        bool quit_ = false;
    };
} // namespace ris
