#include "ris/input/input_manager_base.hpp"

#include <algorithm>
#include <cmath>

namespace ris {

namespace {

constexpr uint32_t pad_bit(GamePadButton button) noexcept {
    const auto i = static_cast<int32_t>(button);
    return (i > 0 && i < game_pad_button_count) ? (1u << static_cast<uint32_t>(i)) : 0u;
}

constexpr uint32_t mouse_bit(MouseButton button) noexcept {
    const auto i = static_cast<int32_t>(button);
    return (i > 0 && i < mouse_button_count) ? (1u << static_cast<uint32_t>(i)) : 0u;
}

float read_pressure(float pressure) noexcept {
    if (!(pressure > 0.0f)) {   // also NaN
        return 0.0f;
    }
    return pressure > 1.0f ? 1.0f : pressure;
}

Vec2 apply_dead_zone(Vec2 stick, float dead_zone) noexcept {
    stick.x = std::clamp(stick.x, -1.0f, 1.0f);
    stick.y = std::clamp(stick.y, -1.0f, 1.0f);
    if (std::isnan(stick.x) || std::isnan(stick.y)) {
        return {0, 0};
    }
    if (std::sqrt(stick.x * stick.x + stick.y * stick.y) < dead_zone) {
        return {0, 0};
    }
    return stick;
}

} // namespace

void InputManagerBase::TouchContact::release_slot() noexcept {
    in_use = false;
    state = TouchLocationState::Invalid;
    has_previous = false;
    has_frame_previous = false;
    same_frame_released = false;
    pressure = 0;
}

InputManagerBase::InputManagerBase()
    : touch_collection_(TouchCollection::default_max_touch_count),
      contacts_(static_cast<size_t>(TouchCollection::default_max_touch_count)) {}

InputManagerBase::~InputManagerBase() = default;

void InputManagerBase::set_thumbstick_dead_zone(float dead_zone) noexcept {
    dead_zone_ = std::isnan(dead_zone) ? dead_zone_ : std::clamp(dead_zone, 0.0f, 1.0f);
}

const GamePadState& InputManagerBase::game_pad_state(int32_t game_pad_index) const noexcept {
    if (game_pad_index < 0 || game_pad_index >= max_game_pad_count) {
        return disconnected_pad_;
    }
    return pads_[static_cast<size_t>(game_pad_index)];
}

GamePadState* InputManagerBase::pending_pad(int32_t index) noexcept {
    if (index < 0 || index >= max_game_pad_count) {
        return nullptr;
    }
    return &pending_pads_[static_cast<size_t>(index)];
}

// ---- keyboard and mouse ------------------------------------------------------------------------

void InputManagerBase::report_key(Key key, bool down) {
    const int32_t i = static_cast<int32_t>(key);
    if (i <= 0 || i >= key_count) {
        return;
    }
    const auto slot = static_cast<size_t>(i);
    if (down) {
        if (!pending_keyboard_.down_[slot]) {
            pending_keyboard_.pressed_[slot] = true;
        }
        pending_keyboard_.down_[slot] = true;
    } else {
        pending_keyboard_.down_[slot] = false;
        pending_keyboard_.released_[slot] = true;
    }
}

void InputManagerBase::report_mouse_move(Vec2 position, Vec2 delta) {
    pending_mouse_.position = position;
    pending_delta_ += delta;
}

void InputManagerBase::report_mouse_button(MouseButton button, bool down, Vec2 position) {
    const uint32_t bit = mouse_bit(button);
    if (bit == 0) {
        return;
    }
    pending_mouse_.position = position;
    if (down) {
        if ((pending_mouse_.down_ & bit) == 0) {
            pending_mouse_.pressed_ |= bit;
        }
        pending_mouse_.down_ |= bit;
    } else {
        pending_mouse_.down_ &= ~bit;
        pending_mouse_.released_ |= bit;
    }
}

void InputManagerBase::report_mouse_wheel(Vec2 delta) {
    pending_wheel_ += delta;
}

void InputManagerBase::report_focus_lost() {
    pending_keyboard_.down_.fill(false);
    pending_mouse_.down_ = 0;
}

void InputManagerBase::handle_event(const Event& e) {
    switch (e.type) {
        case EventType::KeyDown: report_key(key_from_scancode(e.scancode), true); break;
        case EventType::KeyUp: report_key(key_from_scancode(e.scancode), false); break;
        case EventType::MouseMove: report_mouse_move({e.x, e.y}, {e.dx, e.dy}); break;
        case EventType::MouseButtonDown:
            report_mouse_button(mouse_button_from_event_button(e.button), true, {e.x, e.y});
            break;
        case EventType::MouseButtonUp:
            report_mouse_button(mouse_button_from_event_button(e.button), false, {e.x, e.y});
            break;
        case EventType::MouseWheel: report_mouse_wheel({e.dx, e.dy}); break;
        case EventType::WindowFocusLost: report_focus_lost(); break;
        default: break;
    }
}

// ---- gamepads ----------------------------------------------------------------------------------

void InputManagerBase::report_game_pad_connected(int32_t index, bool connected) {
    GamePadState* pad = pending_pad(index);
    if (!pad) {
        return;
    }
    if (connected) {
        pad->connected_ = true;
        return;
    }
    // Unplugged: whatever was held is released, sticks and triggers go back to the centre.
    pad->released_ |= pad->down_;
    pad->down_ = 0;
    pad->sticks_ = {};
    pad->left_trigger_ = pad->right_trigger_ = 0;
    pad->connected_ = false;
}

void InputManagerBase::report_game_pad_button(int32_t index, GamePadButton button, bool down) {
    GamePadState* pad = pending_pad(index);
    const uint32_t bit = pad_bit(button);
    if (!pad || bit == 0) {
        return;
    }
    pad->connected_ = true;
    if (down) {
        if ((pad->down_ & bit) == 0) {
            pad->pressed_ |= bit;
        }
        pad->down_ |= bit;
    } else {
        pad->down_ &= ~bit;
        pad->released_ |= bit;
    }
}

void InputManagerBase::report_game_pad_axes(int32_t index, Vec2 left_stick, Vec2 right_stick, float left_trigger,
                                            float right_trigger) {
    GamePadState* pad = pending_pad(index);
    if (!pad) {
        return;
    }
    pad->connected_ = true;
    pad->sticks_.left = left_stick;
    pad->sticks_.right = right_stick;
    pad->left_trigger_ = read_pressure(left_trigger);
    pad->right_trigger_ = read_pressure(right_trigger);
}

// ---- touch -------------------------------------------------------------------------------------

InputManagerBase::TouchContact* InputManagerBase::find_contact(int32_t id) noexcept {
    for (TouchContact& contact : contacts_) {
        if (contact.in_use && contact.id == id) {
            return &contact;
        }
    }
    return nullptr;
}

InputManagerBase::TouchContact* InputManagerBase::acquire_contact(int32_t id) noexcept {
    for (TouchContact& contact : contacts_) {
        if (!contact.in_use) {
            contact.in_use = true;
            contact.id = id;
            return &contact;
        }
    }
    return nullptr;
}

bool InputManagerBase::is_terminal(const TouchContact& contact) noexcept {
    return contact.state == TouchLocationState::Released || contact.state == TouchLocationState::Cancelled;
}

void InputManagerBase::remember_current_as_previous(TouchContact& contact) noexcept {
    contact.previous_state = contact.state;
    contact.previous_position = contact.position;
    contact.previous_pressure = contact.pressure;
    contact.has_previous = true;
    contact.has_frame_previous = true;
}

void InputManagerBase::note_touch_device(float pressure) {
    int32_t live = 0;
    for (const TouchContact& contact : contacts_) {
        if (contact.in_use && !is_terminal(contact)) {
            ++live;
        }
    }
    const int32_t maximum = std::max({touch_capabilities_.maximum_touch_count(), live, 1});
    const bool has_pressure = touch_capabilities_.has_pressure() || pressure > 0.0f;
    if (touch_capabilities_.is_connected() && touch_capabilities_.maximum_touch_count() == maximum &&
        touch_capabilities_.has_pressure() == has_pressure) {
        return;
    }
    touch_capabilities_.set(true, maximum, has_pressure);
}

void InputManagerBase::set_touch_capabilities(bool is_connected, int32_t maximum_touch_count, bool has_pressure) {
    touch_capabilities_.set(is_connected, std::max(maximum_touch_count, 0), has_pressure);
}

void InputManagerBase::report_touch(TouchPhase phase, int32_t id, Vec2 position, float raw_pressure) {
    const float pressure = read_pressure(raw_pressure);
    switch (phase) {
        case TouchPhase::Begin: {
            if (find_contact(id)) {
                break;
            }
            TouchContact* contact = acquire_contact(id);
            if (!contact) {
                break;
            }
            contact->state = TouchLocationState::Pressed;
            contact->position = position;
            contact->pressure = pressure;
            contact->has_previous = false;
            contact->has_frame_previous = false;
            contact->same_frame_released = false;
            break;
        }
        case TouchPhase::Move: {
            TouchContact* contact = find_contact(id);
            if (!contact || is_terminal(*contact)) {
                break;
            }
            if (!contact->has_frame_previous && position != contact->position) {
                remember_current_as_previous(*contact);
            }
            contact->position = position;
            contact->pressure = pressure;
            break;
        }
        case TouchPhase::End: {
            TouchContact* contact = find_contact(id);
            if (!contact || is_terminal(*contact)) {
                break;
            }
            // A press that also ends before update stays pressed for that read, then ages to released.
            const bool pressed_this_frame = contact->state == TouchLocationState::Pressed;
            if (!contact->has_frame_previous) {
                remember_current_as_previous(*contact);
            }
            contact->position = position;
            // The end of a touch usually reports pressure 0. Keep the last real reading.
            if (pressure > 0.0f) {
                contact->pressure = pressure;
            }
            if (pressed_this_frame) {
                contact->same_frame_released = true;
                break;
            }
            contact->state = TouchLocationState::Released;
            break;
        }
        case TouchPhase::Cancel: {
            TouchContact* contact = find_contact(id);
            if (!contact || contact->state == TouchLocationState::Cancelled) {
                break;
            }
            // A system cancel is not a finger lift, including when the press has not been published yet.
            if (!contact->has_frame_previous) {
                remember_current_as_previous(*contact);
            }
            contact->position = position;
            if (pressure > 0.0f) {
                contact->pressure = pressure;
            }
            contact->same_frame_released = false;
            contact->state = TouchLocationState::Cancelled;
            break;
        }
    }
    note_touch_device(pressure);
}

void InputManagerBase::publish_touches() {
    touch_collection_.clear();
    for (const TouchContact& contact : contacts_) {
        if (!contact.in_use || contact.state == TouchLocationState::Invalid) {
            continue;
        }
        if (contact.has_previous) {
            touch_collection_.try_add(contact.id, contact.state, contact.position.x, contact.position.y,
                                      contact.pressure, contact.previous_state, contact.previous_position.x,
                                      contact.previous_position.y, contact.previous_pressure);
        } else {
            touch_collection_.try_add(contact.id, contact.state, contact.position.x, contact.position.y,
                                      contact.pressure);
        }
    }
}

void InputManagerBase::age_touches() noexcept {
    for (TouchContact& contact : contacts_) {
        if (!contact.in_use) {
            continue;
        }
        if (is_terminal(contact)) {
            contact.release_slot();
            continue;
        }
        remember_current_as_previous(contact);
        if (contact.state == TouchLocationState::Pressed) {
            contact.state = contact.same_frame_released ? TouchLocationState::Released : TouchLocationState::Moved;
        }
        contact.same_frame_released = false;
    }
}

// ---- frame -------------------------------------------------------------------------------------

void InputManagerBase::update() {
    keyboard_ = pending_keyboard_;
    pending_keyboard_.pressed_.fill(false);
    pending_keyboard_.released_.fill(false);

    const Vec2 wheel_total = mouse_.scroll_wheel_position + pending_wheel_;
    mouse_.position = pending_mouse_.position;
    mouse_.delta = pending_delta_;
    mouse_.scroll_wheel_delta = pending_wheel_;
    mouse_.scroll_wheel_position = wheel_total;
    mouse_.down_ = pending_mouse_.down_;
    mouse_.pressed_ = pending_mouse_.pressed_;
    mouse_.released_ = pending_mouse_.released_;
    pending_mouse_.pressed_ = 0;
    pending_mouse_.released_ = 0;
    pending_delta_ = {0, 0};
    pending_wheel_ = {0, 0};

    for (size_t i = 0; i < pads_.size(); ++i) {
        GamePadState& pending = pending_pads_[i];
        pads_[i] = pending;
        pads_[i].sticks_.left = apply_dead_zone(pending.sticks_.left, dead_zone_);
        pads_[i].sticks_.right = apply_dead_zone(pending.sticks_.right, dead_zone_);
        pending.pressed_ = 0;
        pending.released_ = 0;
    }

    publish_touches();
    touch_published_ = true;
}

void InputManagerBase::after_update() {
    if (!touch_published_) {
        return;
    }
    age_touches();
    touch_published_ = false;
}

} // namespace ris
