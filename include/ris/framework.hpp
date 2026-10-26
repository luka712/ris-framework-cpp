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
#include "sprites/sprite_batch.hpp"
#include "ris/rendering/a_graphics_device.hpp"
#include "ris/rendering/texture/a_texture_factory.hpp"
#include "ris/ris.h"
#include "ris/time.hpp"
#include "ris/window_manager.hpp"

#ifdef __cplusplus
    #define EXTERN_API extern "C"
#else
    #define EXTERN_API extern
#endif

namespace ris {
    /// Frame level rendering state, like `IRenderer`.
    class RIS_API Renderer {
    public:
        explicit Renderer(WindowManager &window) : window_(window) {
        }

        /// The colour the back buffer is cleared with at the start of every frame. Black by default.
        Color getClearColor() const { return m_clearColor; }
        void setClearColor(Color color) { m_clearColor = color; }

        /// Drawable size in pixels.
        glm::vec2 back_buffer_size() const { return window_.getWindowBounds(); }

        /// Registers a listener that is called with the new drawable size when the window is resized.
        /// @return its id (1, 2, 3, ...), for `remove_on_resized_listener`.
        uint32_t add_on_resized_listener(std::function<void(glm::vec2)> listener) {
            const uint32_t id = ++s_resizedIds;
            resized_[id] = std::move(listener);
            return id;
        }

        /// Removes a resized listener. @return false for an unknown id.
        bool remove_on_resized_listener(uint32_t id) { return resized_.erase(id) > 0; }

    private:
        friend class Framework;
        static uint32_t s_resizedIds; ///< shared by all renderers, the last id given
        WindowManager &window_;
        Color m_clearColor = Color::black();
        std::map<uint32_t, std::function<void(glm::vec2)> > resized_;
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
        /// TODO: doc comment
        explicit Framework(const RisFrameworkConfiguration &config);

        // TODO: doc comment
        virtual ~Framework();

        // TODO: doc comment
        Framework(const Framework &) = delete;

        WindowManager &windowManager() { return *m_windowManager; }
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
        [[nodiscard]] ATextureFactory &textureFactory() const { return *m_textureFactory; }
        /// The graphics device of the rendering backend of the framework (OpenGL ES): GPU info and features, default
        /// sampler / blend / primitive state, and the creation of samplers, render passes and swap chains. Created
        /// and initialised with the window, see AGraphicsDevice.
        AGraphicsDevice &graphicsDevice() { return *m_graphicsDevice; }
        ASpriteBatch &spriteBatch() const { return *sprite_batch_; }
        const RisFrameworkConfiguration &configuration() const { return config_; }

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
        /// `TimeManager::set_fixed_time_step_ms`) it runs zero or more times per frame, before the update
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
        bool removeOnLoadContentListener(const uint32_t id) { return m_onLoadContentListeners.erase(id) > 0; }
        bool removeOnInitializedListener(const uint32_t id) { return m_onInitializedListeners.erase(id) > 0; }
        bool removeOnUpdateListener(const uint32_t id) { return m_onUpdateListeners.erase(id) > 0; }
        bool removeOnFixedUpdateListener(const uint32_t id) { return m_onFixedUpdateListeners.erase(id) > 0; }
        bool removeOnRenderListener(const uint32_t id) { return m_onRenderListeners.erase(id) > 0; }

        /// Runs the load content and initialized listeners, then the game loop: events, input, update
        /// listeners, clear, render listeners, swap. Returns when the window is closed or `quit()` is called.
        void initialize();

        /// Asks the loop to stop after the current frame.
        void quit() { m_windowManager->quit(); }

    private:
        RisFrameworkConfiguration config_;
        std::unique_ptr<WindowManager> m_windowManager;
        std::unique_ptr<Renderer> renderer_;
        InputManager input_; // declared after m_windowManager: disposed before the window goes away
        TimeManager time_;
        ContentManager content_;
        CameraFactory camera_factory_;
        MaterialFactory material_factory_;
        MeshFactory mesh_factory_;
        ATextureFactory* m_textureFactory;
        AGraphicsDevice* m_graphicsDevice; // after m_windowManager: destroyed before the window goes away
        std::unique_ptr<ASpriteBatch> sprite_batch_;

        // The last id given, one counter per kind of listener, shared by all frameworks (ids start at 1, 0 is never used).
        static uint32_t s_loadContentIds, s_initializedIds, s_updateIds, s_fixedUpdateIds, s_renderIds;

        std::map<uint32_t, std::function<void(Framework &)> > m_onLoadContentListeners, m_onInitializedListeners,
                m_onRenderListeners;
        std::map<uint32_t, std::function<void(Framework &, const GameTime &)> > m_onUpdateListeners,
                m_onFixedUpdateListeners;
    };
} // namespace ris
