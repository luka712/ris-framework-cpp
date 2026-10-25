#include "ris/framework.hpp"

#include <algorithm>
#include <cstdio>

#include "internal.hpp"

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

ListenerId Renderer::add_on_resized_listener(std::function<void(Vec2)> listener) {
    resized_.emplace_back(next_id_, std::move(listener));
    return next_id_++;
}

bool Renderer::remove_on_resized_listener(ListenerId id) {
    auto it = std::find_if(resized_.begin(), resized_.end(), [&](const auto& p) { return p.first == id; });
    if (it == resized_.end()) return false;
    resized_.erase(it);
    return true;
}

namespace {

WindowDesc make_window_desc(const FrameworkConfiguration& c) {
    WindowDesc d;
    d.title = c.title;
    d.width = static_cast<int32_t>(c.back_buffer_size.x);
    d.height = static_cast<int32_t>(c.back_buffer_size.y);
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
    if (!f) throw_error(Result::Io, "could not write screenshot %s", path.c_str());
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

template <class V, class F>
ListenerId add_listener(V& v, ListenerId& next, F f) {
    v.emplace_back(next, std::move(f));
    return next++;
}

template <class V>
bool remove_listener(V& v, ListenerId id) {
    auto it = std::find_if(v.begin(), v.end(), [&](const auto& p) { return p.first == id; });
    if (it == v.end()) return false;
    v.erase(it);
    return true;
}

} // namespace

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
    input_ = create_default_input_manager();
    input_->initialize();
    sprite_batch_ = std::make_unique<SpriteBatch>();
    RIS_LOG_I("ris-framework %s | %s | %s", version_string(), renderer_->device_name(), renderer_->backend_name());
}

Framework::~Framework() = default;

ListenerId Framework::add_on_load_content_listener(std::function<void()> l) { return add_listener(load_content_, next_id_, std::move(l)); }
ListenerId Framework::add_on_initialized_listener(std::function<void()> l) { return add_listener(initialized_, next_id_, std::move(l)); }
ListenerId Framework::add_on_update_listener(std::function<void(const GameTime&)> l) { return add_listener(update_, next_id_, std::move(l)); }
ListenerId Framework::add_on_fixed_update_listener(std::function<void(const GameTime&)> l) { return add_listener(fixed_update_, next_id_, std::move(l)); }
ListenerId Framework::add_on_render_listener(std::function<void()> l) { return add_listener(render_, next_id_, std::move(l)); }
bool Framework::remove_on_load_content_listener(ListenerId id) { return remove_listener(load_content_, id); }
bool Framework::remove_on_initialized_listener(ListenerId id) { return remove_listener(initialized_, id); }
bool Framework::remove_on_update_listener(ListenerId id) { return remove_listener(update_, id); }
bool Framework::remove_on_fixed_update_listener(ListenerId id) { return remove_listener(fixed_update_, id); }
bool Framework::remove_on_render_listener(ListenerId id) { return remove_listener(render_, id); }

void Framework::initialize() {
    quit_ = false;
    time_.prepare_start();

    { auto copy = load_content_; for (auto& [id, l] : copy) l(); }
    { auto copy = initialized_; for (auto& [id, l] : copy) l(); }

    int32_t frame = 0;
    for (;;) {
        Event ev;
        while (window_->poll_event(ev)) {
            if (ev.type == EventType::Quit) quit_ = true;
            if (ev.type == EventType::KeyDown && ev.scancode == static_cast<int32_t>(Key::Escape) && config_.exit_on_escape) quit_ = true;
            if (ev.type == EventType::WindowResized) {
                auto copy = renderer_->resized_;
                const Vec2 size(static_cast<float>(ev.width), static_cast<float>(ev.height));
                for (auto& [id, l] : copy) l(size);
            }
        }
        if (quit_ || (config_.max_frames >= 0 && frame >= config_.max_frames)) break;

        time_.frame_start();
        input_->update();
        if (!fixed_update_.empty()) {
            GameTime step;
            while (time_.consume_fixed_step(step)) { auto copy = fixed_update_; for (auto& [id, l] : copy) l(step); }
        }
        { auto copy = update_; for (auto& [id, l] : copy) l(time_.time()); }
        input_->after_update();

        const Vec2 size = renderer_->back_buffer_size();
        gfx::viewport(0, 0, static_cast<int32_t>(size.x), static_cast<int32_t>(size.y));
        gfx::set_depth_state(true, true);
        gfx::clear(renderer_->clear_color.r, renderer_->clear_color.g, renderer_->clear_color.b, renderer_->clear_color.a);
        { auto copy = render_; for (auto& [id, l] : copy) l(); }

        ++frame;
        if (config_.max_frames >= 0 && frame == config_.max_frames && !config_.screenshot_path.empty()) {
            write_ppm(config_.screenshot_path, static_cast<int32_t>(size.x), static_cast<int32_t>(size.y));
        }
        window_->swap_buffers();
        time_.frame_end();
    }
}

} // namespace ris
