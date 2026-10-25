#include "ris/input.hpp"
#include "ris/time.hpp"

#include <algorithm>

namespace ris {

void TimeManager::prepare_start() {
    start_s_ = last_s_ = time_seconds();
    frame_ = 0;
    time_ = {};
}

void TimeManager::frame_start() {
    ++frame_;
    if (fixed_step_ms_ > 0) {
        time_.delta_time_ms = fixed_step_ms_;
        time_.elapsed_time_ms = static_cast<double>(frame_ - 1) * fixed_step_ms_;
        return;
    }
    const double now = time_seconds();
    time_.delta_time_ms = (now - last_s_) * 1000.0;
    time_.elapsed_time_ms = (now - start_s_) * 1000.0;
    last_s_ = now;
}

void InputManager::handle_event(const Event& e) {
    auto key_index = [](int32_t scancode) { return scancode >= 0 && scancode < static_cast<int32_t>(KeyboardState::kKeys); };
    switch (e.type) {
    case EventType::KeyDown:
        if (key_index(e.scancode)) pending_keyboard_.down_[static_cast<size_t>(e.scancode)] = true;
        break;
    case EventType::KeyUp:
        if (key_index(e.scancode)) {
            pending_keyboard_.down_[static_cast<size_t>(e.scancode)] = false;
            pending_keyboard_.released_[static_cast<size_t>(e.scancode)] = true;
        }
        break;
    case EventType::MouseMove:
        pending_mouse_.position = {e.x, e.y};
        pending_delta_ += Vec2(e.dx, e.dy);
        break;
    case EventType::MouseButtonDown:
        pending_mouse_.position = {e.x, e.y};
        pending_mouse_.down_ |= 1u << (static_cast<uint32_t>(e.button) & 31u);
        break;
    case EventType::MouseButtonUp:
        pending_mouse_.position = {e.x, e.y};
        pending_mouse_.down_ &= ~(1u << (static_cast<uint32_t>(e.button) & 31u));
        pending_mouse_.released_ |= 1u << (static_cast<uint32_t>(e.button) & 31u);
        break;
    case EventType::MouseWheel:
        pending_wheel_ += Vec2(e.dx, e.dy);
        break;
    case EventType::WindowFocusLost:
        pending_keyboard_.down_.fill(false);
        pending_mouse_.down_ = 0;
        break;
    default:
        break;
    }
}

void InputManager::begin_frame() {
    keyboard_ = pending_keyboard_;
    pending_keyboard_.released_.fill(false);

    const Vec2 wheel_total = mouse_.scroll_wheel_position + pending_wheel_;
    mouse_.position = pending_mouse_.position;
    mouse_.delta = pending_delta_;
    mouse_.scroll_wheel_delta = pending_wheel_;
    mouse_.scroll_wheel_position = wheel_total;
    mouse_.down_ = pending_mouse_.down_;
    mouse_.released_ = pending_mouse_.released_;
    pending_mouse_.released_ = 0;
    pending_delta_ = {0, 0};
    pending_wheel_ = {0, 0};
}

} // namespace ris
