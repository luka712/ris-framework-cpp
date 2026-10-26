#include "ris/framework.hpp"

#include <algorithm>
#include <cstdio>
#include <ios>

#include "internal.hpp"
#include "ris/gles/gles_graphics_device.hpp"
#include "ris/gles/texture/gles_texture2d.hpp"
#include "ris/gles/texture/gles_texture_factory.hpp"
#include "ris/rendering/default_graphics_device.hpp"

using namespace ris::detail;

namespace ris {

glm::vec2 WindowManager::size() const {
    int32_t w = 0, h = 0;
    window_.drawable_size(w, h);
    return {static_cast<float>(w), static_cast<float>(h)};
}

glm::vec2 Renderer::back_buffer_size() const {
    int32_t w = 0, h = 0;
    window_.drawable_size(w, h);
    return {static_cast<float>(w), static_cast<float>(h)};
}

uint32_t Renderer::s_resizedIds = 0;

namespace {

/// Calls the listeners of a copy of the map, so a listener may add or remove listeners; a listener removed meanwhile is skipped.
template <class Map, class... Args>
void dispatch(const Map& listeners, Args&&... args) {
    const Map copy = listeners;
    for (const auto& [id, listener] : copy) {
        if (listeners.contains(id)) listener(args...);
    }
}

WindowDesc make_window_desc(const FrameworkConfiguration& c) {
    WindowDesc d;
    d.title = c.title;
    d.width = static_cast<int32_t>(c.backBufferWidth);
    d.height = static_cast<int32_t>(c.backBufferHeight);
    d.flags = WindowFlags::Resizable | WindowFlags::HighDpi;
    d.backend = AngleBackend::Auto;
    d.vsync = true;
    return d;
}

} // namespace

uint32_t Framework::s_loadContentIds = 0;
uint32_t Framework::s_initializedIds = 0;
uint32_t Framework::s_updateIds = 0;
uint32_t Framework::s_fixedUpdateIds = 0;
uint32_t Framework::s_renderIds = 0;

Framework::Framework(const FrameworkConfiguration& config)
    : config_(config),
      content_(*this),
      camera_factory_(*this) {
    window_ = std::make_unique<Window>(make_window_desc(config_));
    window_manager_ = std::make_unique<WindowManager>(*window_);
    renderer_ = std::make_unique<Renderer>(*window_);
    input_.initialize();
    m_textureFactory = new gles::GlesTextureFactory();
    GraphicsDeviceDescriptor graphics_device_descriptor;
    graphics_device_descriptor.samplerFilteringPreset = config_.textureFiltering;
    m_graphicsDevice = new gles::GlesGraphicsDevice(graphics_device_descriptor);
    m_graphicsDevice->initialize();
    sprite_batch_ = std::make_unique<ASpriteBatch>();
    RIS_LOG_I("ris-framework %s | %s | %s", version_string(), renderer_->device_name(), renderer_->backend_name());
}

Framework::~Framework() {
    // The default texture needs the GL context, which the window (a member) owns.
    gles::GlesTexture2D::disposeDefault(*this);

    delete m_textureFactory;
    delete m_graphicsDevice;
}

void Framework::initialize() {
    quit_ = false;
    time_.prepare_start();

    dispatch(m_onLoadContentListeners, *this);
    dispatch(m_onInitializedListeners, *this);

    for (;;) {
        Event ev;
        while (window_->poll_event(ev)) {
            if (ev.type == EventType::Quit) quit_ = true;
            if (ev.type == EventType::KeyDown && ev.scancode == static_cast<int32_t>(Key::Escape) ) quit_ = true;
            if (ev.type == EventType::WindowResized) {
                dispatch(renderer_->resized_, glm::vec2(static_cast<float>(ev.width), static_cast<float>(ev.height)));
            }
        }
        if (quit_) break;

        time_.frame_start();
        input_.update();
        if (!m_onFixedUpdateListeners.empty()) {
            GameTime step;
            while (time_.consume_fixed_step(step)) dispatch(m_onFixedUpdateListeners, *this, step);
        }
        dispatch(m_onUpdateListeners, *this, time_.time());
        input_.after_update();

        const glm::vec2 size = renderer_->back_buffer_size();
        gfx::viewport(0, 0, static_cast<int32_t>(size.x), static_cast<int32_t>(size.y));
        gfx::set_depth_state(true, true);
        const Color clear_color = renderer_->getClearColor();
        gfx::clear(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
        dispatch(m_onRenderListeners, *this);

        window_->swap_buffers();
        m_graphicsDevice->frameEnd();
        time_.frame_end();
    }
}

} // namespace ris
