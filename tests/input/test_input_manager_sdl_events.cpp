// The input manager fed with synthetic SDL events (no display, no devices needed). The tests only use the SDL
// headers for the event structs: the library links SDL privately, so the test executable has no SDL functions.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <SDL3/SDL.h>

#include <memory>
#include <type_traits>

#include <ris/ris.hpp>

using ris::GamePadButton;
using ris::Key;
using ris::MouseButton;
using ris::TouchLocationState;
using ris::InputManager;

namespace {

SDL_Event make_event(SDL_EventType type) {
    SDL_Event e;
    SDL_zero(e);
    e.type = type;
    return e;
}

SDL_Event key_event(SDL_Scancode scancode, bool down) {
    SDL_Event e = make_event(down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP);
    e.key.scancode = scancode;
    e.key.down = down;
    return e;
}

SDL_Event finger_event(SDL_EventType type, SDL_FingerID finger, float x, float y, float pressure = 1.0f) {
    SDL_Event e = make_event(type);
    e.tfinger.touchID = 1;
    e.tfinger.fingerID = finger;
    e.tfinger.x = x;
    e.tfinger.y = y;
    e.tfinger.pressure = pressure;
    return e;
}

SDL_Event pad_button(SDL_JoystickID id, SDL_GamepadButton button, bool down) {
    SDL_Event e = make_event(down ? SDL_EVENT_GAMEPAD_BUTTON_DOWN : SDL_EVENT_GAMEPAD_BUTTON_UP);
    e.gbutton.which = id;
    e.gbutton.button = static_cast<Uint8>(button);
    e.gbutton.down = down;
    return e;
}

SDL_Event pad_axis(SDL_JoystickID id, SDL_GamepadAxis axis, Sint16 value) {
    SDL_Event e = make_event(SDL_EVENT_GAMEPAD_AXIS_MOTION);
    e.gaxis.which = id;
    e.gaxis.axis = static_cast<Uint8>(axis);
    e.gaxis.value = value;
    return e;
}

} // namespace

TEST_CASE("input manager is one concrete class that is neither copyable nor movable") {
    STATIC_REQUIRE_FALSE(std::is_abstract_v<InputManager>);
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<InputManager>);
    STATIC_REQUIRE_FALSE(std::is_move_constructible_v<InputManager>);
    const auto input = std::make_unique<InputManager>();
    REQUIRE_FALSE(input->touch_capabilities().is_connected());
}

TEST_CASE("input manager sdl: ignores events that are not input") {
    InputManager input;
    SDL_Event quit = make_event(SDL_EVENT_QUIT);
    REQUIRE_FALSE(input.handle_native_event(&quit));
    SDL_Event resized = make_event(SDL_EVENT_WINDOW_RESIZED);
    REQUIRE_FALSE(input.handle_native_event(&resized));
    REQUIRE_FALSE(input.handle_native_event(nullptr));
}

TEST_CASE("input manager sdl: keyboard events become keyboard state") {
    InputManager input;
    SDL_Event down = key_event(SDL_SCANCODE_W, true);
    REQUIRE(input.handle_native_event(&down));
    input.update();
    REQUIRE(input.keyboard_state().is_key_down(Key::W));
    REQUIRE(input.keyboard_state().is_key_pressed(Key::W));

    SDL_Event up = key_event(SDL_SCANCODE_W, false);
    input.handle_native_event(&up);
    input.update();
    REQUIRE(input.keyboard_state().is_key_up(Key::W));
    REQUIRE(input.keyboard_state().is_key_released(Key::W));

    SDL_Event other = key_event(SDL_SCANCODE_SPACE, true);
    input.handle_native_event(&other);
    SDL_Event arrow = key_event(SDL_SCANCODE_LEFT, true);
    input.handle_native_event(&arrow);
    input.update();
    REQUIRE(input.keyboard_state().is_key_down(Key::Space));
    REQUIRE(input.keyboard_state().is_key_down(Key::Left));
}

TEST_CASE("input manager sdl: mouse events become mouse state") {
    InputManager input;
    SDL_Event move = make_event(SDL_EVENT_MOUSE_MOTION);
    move.motion.x = 30;
    move.motion.y = 40;
    move.motion.xrel = 3;
    move.motion.yrel = -4;
    REQUIRE(input.handle_native_event(&move));

    SDL_Event press = make_event(SDL_EVENT_MOUSE_BUTTON_DOWN);
    press.button.button = SDL_BUTTON_RIGHT;
    press.button.down = true;
    press.button.x = 30;
    press.button.y = 40;
    input.handle_native_event(&press);

    SDL_Event wheel = make_event(SDL_EVENT_MOUSE_WHEEL);
    wheel.wheel.x = 0;
    wheel.wheel.y = 2;
    input.handle_native_event(&wheel);

    input.update();
    const ris::MouseState& mouse = input.mouse_state();
    REQUIRE(mouse.x() == 30);
    REQUIRE(mouse.y() == 40);
    REQUIRE(mouse.dx() == 3);
    REQUIRE(mouse.dy() == -4);
    REQUIRE(mouse.is_button_down(MouseButton::Right));
    REQUIRE(mouse.is_button_pressed(MouseButton::Right));
    REQUIRE(mouse.is_button_up(MouseButton::Left));
    REQUIRE(mouse.scroll_wheel_delta.y == 2);

    SDL_Event release = press;
    release.type = SDL_EVENT_MOUSE_BUTTON_UP;
    release.button.down = false;
    input.handle_native_event(&release);
    input.update();
    REQUIRE(input.mouse_state().is_button_released(MouseButton::Right));
}

TEST_CASE("input manager sdl: mouse button numbers map to the TS buttons") {
    InputManager input;
    const Uint8 sdl_buttons[] = {SDL_BUTTON_LEFT, SDL_BUTTON_MIDDLE, SDL_BUTTON_RIGHT, SDL_BUTTON_X1, SDL_BUTTON_X2};
    const MouseButton buttons[] = {MouseButton::Left, MouseButton::Middle, MouseButton::Right, MouseButton::X1, MouseButton::X2};
    for (int i = 0; i < 5; ++i) {
        SDL_Event press = make_event(SDL_EVENT_MOUSE_BUTTON_DOWN);
        press.button.button = sdl_buttons[i];
        press.button.down = true;
        input.handle_native_event(&press);
    }
    input.update();
    for (const MouseButton button : buttons) {
        REQUIRE(input.mouse_state().is_button_down(button));
    }
}

TEST_CASE("input manager sdl: focus lost clears held keys") {
    InputManager input;
    SDL_Event down = key_event(SDL_SCANCODE_A, true);
    input.handle_native_event(&down);
    input.update();
    SDL_Event lost = make_event(SDL_EVENT_WINDOW_FOCUS_LOST);
    REQUIRE(input.handle_native_event(&lost));
    input.update();
    REQUIRE(input.keyboard_state().is_key_up(Key::A));
}

TEST_CASE("input manager sdl: finger events become touch locations") {
    InputManager input;
    input.set_touch_surface_size({200, 100});   // no window in a synthetic event

    SDL_Event down = finger_event(SDL_EVENT_FINGER_DOWN, 77, 0.5f, 0.25f, 0.8f);
    REQUIRE(input.handle_native_event(&down));
    input.update();
    REQUIRE(input.touch_capabilities().is_connected());
    REQUIRE(input.touch_collection().count() == 1);
    const ris::TouchLocation* t = input.touch_collection().get(0);
    REQUIRE(t->is_pressed());
    REQUIRE(t->x() == 100);
    REQUIRE(t->y() == 25);
    const int32_t id = t->id;
    input.after_update();

    SDL_Event move = finger_event(SDL_EVENT_FINGER_MOTION, 77, 1.0f, 0.5f);
    input.handle_native_event(&move);
    input.update();
    t = input.touch_collection().find_by_id(id);
    REQUIRE(t != nullptr);
    REQUIRE(t->state() == TouchLocationState::Moved);
    REQUIRE(t->x() == 200);
    REQUIRE(t->try_get_previous_location()->x() == 100);
    input.after_update();

    SDL_Event up = finger_event(SDL_EVENT_FINGER_UP, 77, 1.0f, 0.5f, 0.0f);
    input.handle_native_event(&up);
    input.update();
    REQUIRE(input.touch_collection().is_released(id));
    input.after_update();
    input.update();
    REQUIRE(input.touch_collection().count() == 0);

    // an event of a finger that never went down is ignored
    SDL_Event stray = finger_event(SDL_EVENT_FINGER_MOTION, 5, 0.1f, 0.1f);
    input.handle_native_event(&stray);
    input.update();
    REQUIRE(input.touch_collection().count() == 0);
}

TEST_CASE("input manager sdl: two fingers get different contact ids and a cancel is not a lift") {
    InputManager input;
    SDL_Event a = finger_event(SDL_EVENT_FINGER_DOWN, 1, 0.1f, 0.1f);
    SDL_Event b = finger_event(SDL_EVENT_FINGER_DOWN, 2, 0.9f, 0.9f);
    input.handle_native_event(&a);
    input.handle_native_event(&b);
    input.update();
    REQUIRE(input.touch_collection().count() == 2);
    const int32_t id_a = input.touch_collection().get(0)->id;
    const int32_t id_b = input.touch_collection().get(1)->id;
    REQUIRE(id_a != id_b);
    input.after_update();

    SDL_Event cancel = finger_event(SDL_EVENT_FINGER_CANCELED, 1, 0.1f, 0.1f);
    input.handle_native_event(&cancel);
    input.update();
    REQUIRE(input.touch_collection().is_cancelled(id_a));
    REQUIRE_FALSE(input.touch_collection().is_released(id_a));
    REQUIRE(input.touch_collection().is_down(id_b));
}

TEST_CASE("input manager sdl: gamepad events use lazily assigned slots") {
    InputManager input;
    input.update();
    REQUIRE_FALSE(input.game_pad_state(0).is_connected());   // no pad: everything reports not connected

    SDL_Event button = pad_button(41, SDL_GAMEPAD_BUTTON_SOUTH, true);
    REQUIRE(input.handle_native_event(&button));
    SDL_Event other = pad_button(99, SDL_GAMEPAD_BUTTON_NORTH, true);
    input.handle_native_event(&other);
    input.update();
    REQUIRE(input.game_pad_state(0).is_connected());
    REQUIRE(input.game_pad_state(0).is_button_down(GamePadButton::A));
    REQUIRE(input.game_pad_state(1).is_connected());
    REQUIRE(input.game_pad_state(1).is_button_down(GamePadButton::Y));
    REQUIRE_FALSE(input.game_pad_state(2).is_connected());

    SDL_Event release = pad_button(41, SDL_GAMEPAD_BUTTON_SOUTH, false);
    input.handle_native_event(&release);
    input.update();
    REQUIRE(input.game_pad_state(0).is_button_released(GamePadButton::A));
}

TEST_CASE("input manager sdl: gamepad buttons map to the Xbox layout") {
    InputManager input;
    const struct { SDL_GamepadButton sdl; GamePadButton ours; } map[] = {
        {SDL_GAMEPAD_BUTTON_SOUTH, GamePadButton::A},        {SDL_GAMEPAD_BUTTON_EAST, GamePadButton::B},
        {SDL_GAMEPAD_BUTTON_WEST, GamePadButton::X},         {SDL_GAMEPAD_BUTTON_NORTH, GamePadButton::Y},
        {SDL_GAMEPAD_BUTTON_BACK, GamePadButton::Back},      {SDL_GAMEPAD_BUTTON_START, GamePadButton::Start},
        {SDL_GAMEPAD_BUTTON_GUIDE, GamePadButton::BigButton},
        {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, GamePadButton::LeftShoulder},
        {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, GamePadButton::RightShoulder},
        {SDL_GAMEPAD_BUTTON_LEFT_STICK, GamePadButton::LeftStick},
        {SDL_GAMEPAD_BUTTON_RIGHT_STICK, GamePadButton::RightStick},
        {SDL_GAMEPAD_BUTTON_DPAD_UP, GamePadButton::DPadUp},  {SDL_GAMEPAD_BUTTON_DPAD_DOWN, GamePadButton::DPadDown},
        {SDL_GAMEPAD_BUTTON_DPAD_LEFT, GamePadButton::DPadLeft}, {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, GamePadButton::DPadRight},
    };
    for (const auto& m : map) {
        SDL_Event e = pad_button(1, m.sdl, true);
        input.handle_native_event(&e);
    }
    SDL_Event unmapped = pad_button(1, SDL_GAMEPAD_BUTTON_TOUCHPAD, true);
    input.handle_native_event(&unmapped);
    input.update();
    for (const auto& m : map) {
        REQUIRE(input.game_pad_state(0).is_button_down(m.ours));
    }
}

TEST_CASE("input manager sdl: gamepad axes: y is flipped, triggers are 0 to 1, the dead zone applies") {
    InputManager input;
    SDL_Event lx = pad_axis(5, SDL_GAMEPAD_AXIS_LEFTX, 32767);
    SDL_Event ly = pad_axis(5, SDL_GAMEPAD_AXIS_LEFTY, -32768);   // SDL: up is negative
    SDL_Event rx = pad_axis(5, SDL_GAMEPAD_AXIS_RIGHTX, 1000);   // inside the dead zone
    SDL_Event lt = pad_axis(5, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 32767);
    SDL_Event rt = pad_axis(5, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0);
    for (SDL_Event* e : {&lx, &ly, &rx, &lt, &rt}) {
        REQUIRE(input.handle_native_event(e));
    }
    input.update();
    const ris::GamePadState& pad = input.game_pad_state(0);
    REQUIRE(pad.is_connected());
    REQUIRE_THAT(pad.thumb_sticks().left.x, Catch::Matchers::WithinAbs(1.0, 1e-3));
    REQUIRE_THAT(pad.thumb_sticks().left.y, Catch::Matchers::WithinAbs(1.0, 1e-3));
    REQUIRE(pad.thumb_sticks().right == glm::vec2(0, 0));
    REQUIRE_THAT(pad.left_trigger(), Catch::Matchers::WithinAbs(1.0, 1e-3));
    REQUIRE(pad.right_trigger() == 0.0f);
}

TEST_CASE("input manager sdl: gamepad removal frees the slot") {
    InputManager input;
    SDL_Event button = pad_button(8, SDL_GAMEPAD_BUTTON_EAST, true);
    input.handle_native_event(&button);
    input.update();
    REQUIRE(input.game_pad_state(0).is_connected());

    SDL_Event removed = make_event(SDL_EVENT_GAMEPAD_REMOVED);
    removed.gdevice.which = 8;
    REQUIRE(input.handle_native_event(&removed));
    input.update();
    REQUIRE_FALSE(input.game_pad_state(0).is_connected());
    REQUIRE(input.game_pad_state(0).is_button_released(GamePadButton::B));

    SDL_Event again = pad_button(9, SDL_GAMEPAD_BUTTON_EAST, true);   // the slot is reused
    input.handle_native_event(&again);
    input.update();
    REQUIRE(input.game_pad_state(0).is_connected());
}

TEST_CASE("input manager sdl: gamepad added for a device that cannot be opened does not connect") {
    InputManager input;
    input.initialize();
    SDL_Event added = make_event(SDL_EVENT_GAMEPAD_ADDED);
    added.gdevice.which = 123456;   // no such joystick
    REQUIRE(input.handle_native_event(&added));
    input.update();
    REQUIRE_FALSE(input.game_pad_state(0).is_connected());
    input.dispose();
    input.dispose();   // safe twice
}

TEST_CASE("input manager sdl: initialize and dispose are safe to repeat") {
    InputManager input;
    REQUIRE_NOTHROW(input.initialize());
    REQUIRE_NOTHROW(input.initialize());
    REQUIRE_NOTHROW(input.update());
    REQUIRE_NOTHROW(input.after_update());
    REQUIRE_NOTHROW(input.dispose());
    REQUIRE_NOTHROW(input.dispose());
}
