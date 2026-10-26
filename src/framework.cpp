#include "ris/framework.hpp"

#include <algorithm>
#include <cstdio>
#include <ios>

#include <SDL3/SDL.h>

#include "internal.hpp"
#include "ris/gles/gles_graphics_device.hpp"
#include "ris/gles/texture/gles_texture2d.hpp"
#include "ris/gles/texture/gles_texture_factory.hpp"
#include "ris/rendering/default_graphics_device.hpp"
#include "ris/window_manager.hpp"

using namespace ris::detail;

namespace ris {

uint32_t Renderer::s_resizedIds = 0;

namespace {

WindowDescriptor make_window_desc(const RisFrameworkConfiguration& c) {
    WindowDescriptor d;
    d.title = c.title;
    d.width = static_cast<int32_t>(c.backBufferWidth);
    d.height = static_cast<int32_t>(c.backBufferHeight);
    return d;
}

} // namespace

uint32_t Framework::s_loadContentIds = 0;
uint32_t Framework::s_initializedIds = 0;
uint32_t Framework::s_updateIds = 0;
uint32_t Framework::s_fixedUpdateIds = 0;
uint32_t Framework::s_renderIds = 0;

Framework::Framework(const RisFrameworkConfiguration& config)
    : config_(config),
      content_(*this),
      camera_factory_(*this) {
    auto windowManager = std::make_unique<WindowManager>(make_window_desc(config_));
    windowManager->initializeForGles();
    windowManager->addOnSdlEventListener([this](const SDL_Event& event) { input_.handle_native_event(&event); });
    m_windowManager = std::move(windowManager);
    renderer_ = std::make_unique<Renderer>(*m_windowManager);
    input_.initialize();
    m_textureFactory = new gles::GlesTextureFactory();
    GraphicsDeviceDescriptor graphics_device_descriptor;
    graphics_device_descriptor.samplerFilteringPreset = config_.textureFiltering;
    m_graphicsDevice = new gles::GlesGraphicsDevice(*this, graphics_device_descriptor);
    m_graphicsDevice->initialize();
    sprite_batch_ = std::make_unique<ASpriteBatch>();

    m_windowManager->addOnResizeListener([this](WindowManager&, const glm::vec2 size) { dispatch(renderer_->resized_, size); });
    m_windowManager->addOnUpdateListener([this] {
        time_.frame_start();
        input_.update();
        if (input_.keyboard_state().is_key_pressed(Key::Escape)) m_windowManager->quit();
        if (!m_onFixedUpdateListeners.empty()) {
            GameTime step;
            while (time_.consume_fixed_step(step)) dispatch(m_onFixedUpdateListeners, *this, step);
        }
        dispatch(m_onUpdateListeners, *this, time_.time());
        input_.after_update();
    });
    m_windowManager->addOnRenderListener([this] {
        const glm::vec2 size = renderer_->back_buffer_size();
        gfx::viewport(0, 0, static_cast<int32_t>(size.x), static_cast<int32_t>(size.y));
        gfx::set_depth_state(true, true);
        const Color clear_color = renderer_->getClearColor();
        gfx::clear(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
        dispatch(m_onRenderListeners, *this);

        m_graphicsDevice->frameEnd();
        time_.frame_end();
    });
    RIS_LOG_I("ris-framework %s", version_string());
}

Framework::~Framework() {
    // The default texture needs the GL context, which the window (a member) owns.
    gles::GlesTexture2D::disposeDefault(*this);

    delete m_textureFactory;
    delete m_graphicsDevice;
}

void Framework::initialize() {
    time_.prepare_start();

    dispatch(m_onLoadContentListeners, *this);
    dispatch(m_onInitializedListeners, *this);

    m_windowManager->runEventLoop();
}

} // namespace ris
