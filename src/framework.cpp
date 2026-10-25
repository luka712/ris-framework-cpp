#include "ris/framework.hpp"

#include <algorithm>
#include <cstdio>
#include <ios>

#include "internal.hpp"
#include "ris/rendering/default_graphics_device.hpp"
#include "ris/rendering/default_texture_factory.hpp"

using namespace ris::detail;

namespace ris {

Vec2 WindowManager::size() const {
    int32_t w = 0, h = 0;
    window_.drawable_size(w, h);
    return {static_cast<float>(w), static_cast<float>(h)};
}

Vec2 Renderer::back_buffer_size() const {
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
    d.width = static_cast<int32_t>(c.backBufferSize.x);
    d.height = static_cast<int32_t>(c.backBufferSize.y);
    d.flags = WindowFlags::None;
    if (c.resizable) d.flags = d.flags | WindowFlags::Resizable;
    if (c.high_dpi) d.flags = d.flags | WindowFlags::HighDpi;
    if (c.hidden) d.flags = d.flags | WindowFlags::Hidden;
    d.backend = c.backend;
    d.vsync = c.vsync;
    return d;
}

void write_ppm(const std::string& path, int32_t w, int32_t h) {
    const std::vector<uint8_t> px = gfx::read_pixels(0, 0, w, h);
    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) throw std::ios_base::failure(std::string("could not write screenshot ") + path);
    std::fprintf(f, "P6\n%d %d\n255\n", w, h);
    std::vector<uint8_t> row(static_cast<size_t>(w) * 3);
    for (int32_t y = h - 1; y >= 0; --y) {   // OpenGL's origin is the bottom left
        for (int32_t x = 0; x < w; ++x) {
            const size_t s = (static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x)) * 4;
            row[static_cast<size_t>(x) * 3 + 0] = px[s + 0];
            row[static_cast<size_t>(x) * 3 + 1] = px[s + 1];
            row[static_cast<size_t>(x) * 3 + 2] = px[s + 2];
        }
        std::fwrite(row.data(), 1, row.size(), f);
    }
    std::fclose(f);
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
    if (config_.max_frames >= 0) time_.set_deterministic_step_ms(1000.0 / 60.0);
    time_.set_target_frames_per_second(config_.target_frames_per_second);
    time_.set_fixed_time_step_ms(config_.fixed_time_step_ms);
    window_ = std::make_unique<Window>(make_window_desc(config_));
    window_manager_ = std::make_unique<WindowManager>(*window_);
    renderer_ = std::make_unique<Renderer>(*window_);
    input_.initialize();
    texture_factory_ = create_default_texture_factory();
    GraphicsDeviceDescriptor graphics_device_descriptor;
    graphics_device_descriptor.samplerFilteringPreset = config_.textureFiltering;
    graphics_device_ = create_default_graphics_device(graphics_device_descriptor);
    graphics_device_->initialize();
    sprite_batch_ = std::make_unique<SpriteBatch>();
    RIS_LOG_I("ris-framework %s | %s | %s", version_string(), renderer_->device_name(), renderer_->backend_name());
}

Framework::~Framework() = default;

void Framework::initialize() {
    quit_ = false;
    time_.prepare_start();

    dispatch(m_onLoadContentListeners, *this);
    dispatch(m_onInitializedListeners, *this);

    int32_t frame = 0;
    for (;;) {
        Event ev;
        while (window_->poll_event(ev)) {
            if (ev.type == EventType::Quit) quit_ = true;
            if (ev.type == EventType::KeyDown && ev.scancode == static_cast<int32_t>(Key::Escape) && config_.exit_on_escape) quit_ = true;
            if (ev.type == EventType::WindowResized) {
                dispatch(renderer_->resized_, Vec2(static_cast<float>(ev.width), static_cast<float>(ev.height)));
            }
        }
        if (quit_ || (config_.max_frames >= 0 && frame >= config_.max_frames)) break;

        time_.frame_start();
        input_.update();
        if (!m_onFixedUpdateListeners.empty()) {
            GameTime step;
            while (time_.consume_fixed_step(step)) dispatch(m_onFixedUpdateListeners, *this, step);
        }
        dispatch(m_onUpdateListeners, *this, time_.time());
        input_.after_update();

        const Vec2 size = renderer_->back_buffer_size();
        gfx::viewport(0, 0, static_cast<int32_t>(size.x), static_cast<int32_t>(size.y));
        gfx::set_depth_state(true, true);
        const Color clear_color = renderer_->getClearColor();
        gfx::clear(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
        dispatch(m_onRenderListeners, *this);

        ++frame;
        if (config_.max_frames >= 0 && frame == config_.max_frames && !config_.screenshot_path.empty()) {
            write_ppm(config_.screenshot_path, static_cast<int32_t>(size.x), static_cast<int32_t>(size.y));
        }
        window_->swap_buffers();
        graphics_device_->frameEnd();
        time_.frame_end();
    }
}

} // namespace ris
