#include "ris/input/input_manager.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "internal.hpp"

using namespace ris::detail;

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

glm::vec2 apply_dead_zone(glm::vec2 stick, float dead_zone) noexcept {
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

namespace {

GamePadButton map_gamepad_button(uint8_t button) noexcept {
    switch (static_cast<SDL_GamepadButton>(button)) {
        case SDL_GAMEPAD_BUTTON_SOUTH: return GamePadButton::A;
        case SDL_GAMEPAD_BUTTON_EAST: return GamePadButton::B;
        case SDL_GAMEPAD_BUTTON_WEST: return GamePadButton::X;
        case SDL_GAMEPAD_BUTTON_NORTH: return GamePadButton::Y;
        case SDL_GAMEPAD_BUTTON_BACK: return GamePadButton::Back;
        case SDL_GAMEPAD_BUTTON_GUIDE: return GamePadButton::BigButton;
        case SDL_GAMEPAD_BUTTON_START: return GamePadButton::Start;
        case SDL_GAMEPAD_BUTTON_LEFT_STICK: return GamePadButton::LeftStick;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return GamePadButton::RightStick;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return GamePadButton::LeftShoulder;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return GamePadButton::RightShoulder;
        case SDL_GAMEPAD_BUTTON_DPAD_UP: return GamePadButton::DPadUp;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return GamePadButton::DPadDown;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return GamePadButton::DPadLeft;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return GamePadButton::DPadRight;
        default: return GamePadButton::None;
    }
}

/// -1 to 1 for sticks, from SDL's signed 16 bit axis range.
float normalise_axis(int16_t value) noexcept {
    return value < 0 ? static_cast<float>(value) / 32768.0f : static_cast<float>(value) / 32767.0f;
}

} // namespace

struct InputManager::Impl {
    struct PadSlot {
        SDL_JoystickID id = 0;          // 0 = free
        SDL_Gamepad* gamepad = nullptr; // opened by us; null when the id was only seen in events
        float left_x = 0, left_y = 0, right_x = 0, right_y = 0, left_trigger = 0, right_trigger = 0;
    };
    struct Finger {
        SDL_FingerID finger;
        int32_t contact_id;
    };

    bool initialised = false;
    bool gamepad_subsystem = false;
    std::array<PadSlot, static_cast<size_t>(max_game_pad_count)> pads;
    std::vector<Finger> fingers;
    int32_t next_touch_id = 1;
    glm::vec2 touch_surface{1, 1};

    int find_pad(SDL_JoystickID id) const noexcept {
        for (size_t i = 0; i < pads.size(); ++i) {
            if (pads[i].id == id) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    int free_pad() const noexcept { return find_pad(0); }
};


void InputManager::TouchContact::release_slot() noexcept {
    in_use = false;
    state = TouchLocationState::Invalid;
    has_previous = false;
    has_frame_previous = false;
    same_frame_released = false;
    pressure = 0;
}

InputManager::InputManager()
    : touch_collection_(TouchCollection::default_max_touch_count),
      contacts_(static_cast<size_t>(TouchCollection::default_max_touch_count)),
      impl_(std::make_unique<Impl>()) {}

InputManager::~InputManager() {
    dispose();
}

void InputManager::set_thumbstick_dead_zone(float dead_zone) noexcept {
    dead_zone_ = std::isnan(dead_zone) ? dead_zone_ : std::clamp(dead_zone, 0.0f, 1.0f);
}

const GamePadState& InputManager::game_pad_state(int32_t game_pad_index) const noexcept {
    if (game_pad_index < 0 || game_pad_index >= max_game_pad_count) {
        return disconnected_pad_;
    }
    return pads_[static_cast<size_t>(game_pad_index)];
}

GamePadState* InputManager::pending_pad(int32_t index) noexcept {
    if (index < 0 || index >= max_game_pad_count) {
        return nullptr;
    }
    return &pending_pads_[static_cast<size_t>(index)];
}

// ---- keyboard and mouse ------------------------------------------------------------------------

void InputManager::report_key(Key key, bool down) {
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

void InputManager::report_mouse_move(glm::vec2 position, glm::vec2 delta) {
    pending_mouse_.position = position;
    pending_delta_ += delta;
}

void InputManager::report_mouse_button(MouseButton button, bool down, glm::vec2 position) {
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

void InputManager::report_mouse_wheel(glm::vec2 delta) {
    pending_wheel_ += delta;
}

void InputManager::report_focus_lost() {
    pending_keyboard_.down_.fill(false);
    pending_mouse_.down_ = 0;
}

void InputManager::handle_event(const Event& e) {
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

void InputManager::report_game_pad_connected(int32_t index, bool connected) {
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

void InputManager::report_game_pad_button(int32_t index, GamePadButton button, bool down) {
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

void InputManager::report_game_pad_axes(int32_t index, glm::vec2 left_stick, glm::vec2 right_stick, float left_trigger,
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

InputManager::TouchContact* InputManager::find_contact(int32_t id) noexcept {
    for (TouchContact& contact : contacts_) {
        if (contact.in_use && contact.id == id) {
            return &contact;
        }
    }
    return nullptr;
}

InputManager::TouchContact* InputManager::acquire_contact(int32_t id) noexcept {
    for (TouchContact& contact : contacts_) {
        if (!contact.in_use) {
            contact.in_use = true;
            contact.id = id;
            return &contact;
        }
    }
    return nullptr;
}

bool InputManager::is_terminal(const TouchContact& contact) noexcept {
    return contact.state == TouchLocationState::Released || contact.state == TouchLocationState::Cancelled;
}

void InputManager::remember_current_as_previous(TouchContact& contact) noexcept {
    contact.previous_state = contact.state;
    contact.previous_position = contact.position;
    contact.previous_pressure = contact.pressure;
    contact.has_previous = true;
    contact.has_frame_previous = true;
}

void InputManager::note_touch_device(float pressure) {
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

void InputManager::set_touch_capabilities(bool is_connected, int32_t maximum_touch_count, bool has_pressure) {
    touch_capabilities_.set(is_connected, std::max(maximum_touch_count, 0), has_pressure);
}

void InputManager::report_touch(TouchPhase phase, int32_t id, glm::vec2 position, float raw_pressure) {
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

void InputManager::publish_touches() {
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

void InputManager::age_touches() noexcept {
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

void InputManager::update() {
    keyboard_ = pending_keyboard_;
    pending_keyboard_.pressed_.fill(false);
    pending_keyboard_.released_.fill(false);

    const glm::vec2 wheel_total = mouse_.scroll_wheel_position + pending_wheel_;
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

void InputManager::after_update() {
    if (!touch_published_) {
        return;
    }
    age_touches();
    touch_published_ = false;
}

// ---- SDL3: lifecycle and native events ---------------------------------------------------------

void InputManager::set_touch_surface_size(glm::vec2 size) noexcept {
    if (size.x > 0 && size.y > 0) {
        impl_->touch_surface = size;
    }
}

void InputManager::initialize() {
    if (impl_->initialised) {
        return;
    }
    impl_->initialised = true;

    // The gamepad subsystem is optional: without it (no joystick support, restricted environment) input
    // still works and every gamepad reports "not connected".
    if (SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
        impl_->gamepad_subsystem = true;
        int count = 0;
        SDL_JoystickID* ids = SDL_GetGamepads(&count);
        if (ids) {
            for (int i = 0; i < count; ++i) {
                SDL_Event added;
                SDL_zero(added);
                added.type = SDL_EVENT_GAMEPAD_ADDED;
                added.gdevice.which = ids[i];
                handle_native_event(&added);
            }
            SDL_free(ids);
        }
    } else {
        RIS_LOG_W("SDL gamepad subsystem unavailable, gamepads are disabled: %s", SDL_GetError());
    }

    int touch_count = 0;
    SDL_TouchID* touch_devices = SDL_GetTouchDevices(&touch_count);
    if (touch_devices) {
        if (touch_count > 0) {
            set_touch_capabilities(true, TouchCollection::default_max_touch_count, false);
        }
        SDL_free(touch_devices);
    }
}

void InputManager::dispose() {
    if (!impl_->initialised) {
        return;
    }
    impl_->initialised = false;
    if (impl_->gamepad_subsystem) {
        for (Impl::PadSlot& slot : impl_->pads) {
            if (slot.gamepad) {
                SDL_CloseGamepad(slot.gamepad);
            }
            slot = {};
        }
        SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
        impl_->gamepad_subsystem = false;
    }
    impl_->fingers.clear();
}

bool InputManager::handle_native_event(const void* native_event) {
    if (!native_event) {
        return false;
    }
    const SDL_Event& e = *static_cast<const SDL_Event*>(native_event);
    switch (e.type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            report_key(key_from_scancode(static_cast<int32_t>(e.key.scancode)), e.type == SDL_EVENT_KEY_DOWN);
            return true;
        case SDL_EVENT_MOUSE_MOTION:
            report_mouse_move({e.motion.x, e.motion.y}, {e.motion.xrel, e.motion.yrel});
            return true;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            report_mouse_button(mouse_button_from_event_button(e.button.button), e.type == SDL_EVENT_MOUSE_BUTTON_DOWN,
                                {e.button.x, e.button.y});
            return true;
        case SDL_EVENT_MOUSE_WHEEL:
            report_mouse_wheel({e.wheel.x, e.wheel.y});
            return true;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            report_focus_lost();
            return true;

        case SDL_EVENT_FINGER_DOWN:
        case SDL_EVENT_FINGER_MOTION:
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_CANCELED: {
            const SDL_TouchFingerEvent& f = e.tfinger;
            glm::vec2 surface = impl_->touch_surface;
            if (SDL_Window* window = SDL_GetWindowFromID(f.windowID)) {
                int w = 0, h = 0;
                if (SDL_GetWindowSize(window, &w, &h) && w > 0 && h > 0) {
                    surface = {static_cast<float>(w), static_cast<float>(h)};
                }
            }
            const glm::vec2 position{f.x * surface.x, f.y * surface.y};

            auto& fingers = impl_->fingers;
            auto it = std::find_if(fingers.begin(), fingers.end(), [&](const Impl::Finger& x) { return x.finger == f.fingerID; });
            if (e.type == SDL_EVENT_FINGER_DOWN) {
                if (it == fingers.end()) {
                    const int32_t id = impl_->next_touch_id++;
                    if (impl_->next_touch_id <= 0) {
                        impl_->next_touch_id = 1;
                    }
                    fingers.push_back({f.fingerID, id});
                    report_touch(TouchPhase::Begin, id, position, f.pressure);
                }
            } else if (it != fingers.end()) {
                const int32_t id = it->contact_id;
                if (e.type == SDL_EVENT_FINGER_MOTION) {
                    report_touch(TouchPhase::Move, id, position, f.pressure);
                } else {
                    report_touch(e.type == SDL_EVENT_FINGER_UP ? TouchPhase::End : TouchPhase::Cancel, id, position,
                                 f.pressure);
                    fingers.erase(it);
                }
            }
            return true;
        }

        case SDL_EVENT_GAMEPAD_ADDED: {
            const SDL_JoystickID id = e.gdevice.which;
            if (impl_->find_pad(id) >= 0) {
                return true;
            }
            const int slot = impl_->free_pad();
            if (slot < 0) {
                return true;   // more than max_game_pad_count gamepads: the extra ones are ignored
            }
            SDL_Gamepad* gamepad = impl_->gamepad_subsystem ? SDL_OpenGamepad(id) : nullptr;
            if (!gamepad) {
                RIS_LOG_W("could not open gamepad %u: %s", static_cast<unsigned>(id), SDL_GetError());
                return true;
            }
            impl_->pads[static_cast<size_t>(slot)] = {};
            impl_->pads[static_cast<size_t>(slot)].id = id;
            impl_->pads[static_cast<size_t>(slot)].gamepad = gamepad;
            report_game_pad_connected(slot, true);
            return true;
        }
        case SDL_EVENT_GAMEPAD_REMOVED: {
            const int slot = impl_->find_pad(e.gdevice.which);
            if (slot >= 0) {
                Impl::PadSlot& pad = impl_->pads[static_cast<size_t>(slot)];
                if (pad.gamepad) {
                    SDL_CloseGamepad(pad.gamepad);
                }
                pad = {};
                report_game_pad_connected(slot, false);
            }
            return true;
        }
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
        case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
            // Events of a gamepad that was not announced (opened by the application itself) take a free slot.
            const SDL_JoystickID id = e.type == SDL_EVENT_GAMEPAD_AXIS_MOTION ? e.gaxis.which : e.gbutton.which;
            int slot = impl_->find_pad(id);
            if (slot < 0) {
                slot = impl_->free_pad();
                if (slot < 0) {
                    return true;
                }
                impl_->pads[static_cast<size_t>(slot)] = {};
                impl_->pads[static_cast<size_t>(slot)].id = id;
                report_game_pad_connected(slot, true);
            }
            Impl::PadSlot& pad = impl_->pads[static_cast<size_t>(slot)];
            if (e.type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
                const int16_t value = e.gaxis.value;
                switch (static_cast<SDL_GamepadAxis>(e.gaxis.axis)) {
                    case SDL_GAMEPAD_AXIS_LEFTX: pad.left_x = normalise_axis(value); break;
                    case SDL_GAMEPAD_AXIS_LEFTY: pad.left_y = -normalise_axis(value); break;   // y up
                    case SDL_GAMEPAD_AXIS_RIGHTX: pad.right_x = normalise_axis(value); break;
                    case SDL_GAMEPAD_AXIS_RIGHTY: pad.right_y = -normalise_axis(value); break;
                    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER: pad.left_trigger = normalise_axis(value); break;
                    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: pad.right_trigger = normalise_axis(value); break;
                    default: return true;
                }
                report_game_pad_axes(slot, {pad.left_x, pad.left_y}, {pad.right_x, pad.right_y}, pad.left_trigger,
                                     pad.right_trigger);
            } else {
                report_game_pad_button(slot, map_gamepad_button(e.gbutton.button), e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
            }
            return true;
        }
        default:
            return false;
    }
}

} // namespace ris
