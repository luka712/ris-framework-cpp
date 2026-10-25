#include "ris/sdl3/input/sdl3_input_manager.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <vector>

#include "internal.hpp"
#include "sdl3/sdl3_event_tap.hpp"

using namespace ris::detail;

namespace ris::sdl3 {

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

struct Sdl3InputManager::Impl {
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
    Vec2 touch_surface{1, 1};

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

Sdl3InputManager::Sdl3InputManager() : impl_(std::make_unique<Impl>()) {}

Sdl3InputManager::~Sdl3InputManager() {
    dispose();
}

void Sdl3InputManager::set_touch_surface_size(Vec2 size) noexcept {
    if (size.x > 0 && size.y > 0) {
        impl_->touch_surface = size;
    }
}

void Sdl3InputManager::initialize() {
    if (impl_->initialised) {
        return;
    }
    impl_->initialised = true;
    detail::add_event_tap(this, [this](const SDL_Event& event) { handle_native_event(&event); });

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

void Sdl3InputManager::dispose() {
    if (!impl_->initialised) {
        return;
    }
    impl_->initialised = false;
    detail::remove_event_tap(this);
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

bool Sdl3InputManager::handle_native_event(const void* native_event) {
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
            Vec2 surface = impl_->touch_surface;
            if (SDL_Window* window = SDL_GetWindowFromID(f.windowID)) {
                int w = 0, h = 0;
                if (SDL_GetWindowSize(window, &w, &h) && w > 0 && h > 0) {
                    surface = {static_cast<float>(w), static_cast<float>(h)};
                }
            }
            const Vec2 position{f.x * surface.x, f.y * surface.y};

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

} // namespace ris::sdl3
