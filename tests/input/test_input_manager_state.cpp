// State tracking of the input manager (keyboard, mouse, gamepad), driven through its public `report_*` functions and
// `handle_event`: no SDL event, no window, no device.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <memory>

#include <ris/input.hpp>

using ris::GamePadButton;
using ris::Key;
using ris::MouseButton;
using ris::InputManager;

TEST_CASE("update, after_update and dispose are safe on a manager that was never initialized") {
    InputManager input;
    REQUIRE_NOTHROW(input.update());
    REQUIRE_NOTHROW(input.after_update());
    REQUIRE_NOTHROW(input.dispose());
    REQUIRE_NOTHROW(input.dispose());
    REQUIRE_FALSE(input.game_pad_state().is_connected());
    REQUIRE_FALSE(input.touch_capabilities().is_connected());
}

TEST_CASE("keyboard state is published per frame") {
    InputManager input;
    input.report_key(Key::W, true);
    REQUIRE_FALSE(input.keyboard_state().is_key_down(Key::W));   // collected, not published yet
    input.update();
    REQUIRE(input.keyboard_state().is_key_down(Key::W));
    REQUIRE(input.keyboard_state().is_key_pressed(Key::W));
    REQUIRE_FALSE(input.keyboard_state().is_key_up(Key::W));

    input.update();   // still held: pressed lasts one frame only
    REQUIRE(input.keyboard_state().is_key_down(Key::W));
    REQUIRE_FALSE(input.keyboard_state().is_key_pressed(Key::W));

    input.report_key(Key::W, true);   // auto repeat does not press again
    input.update();
    REQUIRE_FALSE(input.keyboard_state().is_key_pressed(Key::W));

    input.report_key(Key::W, false);
    input.update();
    REQUIRE(input.keyboard_state().is_key_up(Key::W));
    REQUIRE(input.keyboard_state().is_key_released(Key::W));
    input.update();
    REQUIRE_FALSE(input.keyboard_state().is_key_released(Key::W));
}

TEST_CASE("a key pressed and released within one frame is reported as pressed and released") {
    InputManager input;
    input.report_key(Key::Space, true);
    input.report_key(Key::Space, false);
    input.update();
    REQUIRE(input.keyboard_state().is_key_up(Key::Space));
    REQUIRE(input.keyboard_state().is_key_pressed(Key::Space));
    REQUIRE(input.keyboard_state().is_key_released(Key::Space));
}

TEST_CASE("unknown keys are ignored") {
    InputManager input;
    input.report_key(Key::None, true);
    input.report_key(static_cast<Key>(100000), true);
    input.report_key(static_cast<Key>(-4), true);
    input.update();
    REQUIRE_FALSE(input.keyboard_state().is_key_down(Key::None));
    REQUIRE_FALSE(input.keyboard_state().is_key_down(static_cast<Key>(100000)));
}

TEST_CASE("key from scancode keeps the HID values") {
    REQUIRE(ris::key_from_scancode(static_cast<int32_t>(Key::W)) == Key::W);
    REQUIRE(ris::key_from_scancode(-1) == Key::None);
    REQUIRE(ris::key_from_scancode(ris::key_count) == Key::None);
}

TEST_CASE("mouse state accumulates motion and wheel per frame") {
    InputManager input;
    input.report_mouse_move({5, 6}, {2, 3});
    input.report_mouse_move({7, 6}, {2, 0});
    input.report_mouse_wheel({0, 1});
    input.update();
    REQUIRE(input.mouse_state().x() == 7);
    REQUIRE(input.mouse_state().y() == 6);
    REQUIRE(input.mouse_state().dx() == 4);
    REQUIRE(input.mouse_state().dy() == 3);
    REQUIRE(input.mouse_state().scroll_wheel_delta.y == 1);
    REQUIRE(input.mouse_state().scroll_wheel_position.y == 1);

    input.update();   // delta resets, position and the wheel total stay
    REQUIRE(input.mouse_state().dx() == 0);
    REQUIRE(input.mouse_state().x() == 7);
    REQUIRE(input.mouse_state().scroll_wheel_delta.y == 0);
    REQUIRE(input.mouse_state().scroll_wheel_position.y == 1);
}

TEST_CASE("mouse buttons are down, pressed and released") {
    InputManager input;
    input.report_mouse_button(MouseButton::Left, true, {1, 2});
    input.update();
    REQUIRE(input.mouse_state().is_button_down(MouseButton::Left));
    REQUIRE(input.mouse_state().is_button_pressed(MouseButton::Left));
    REQUIRE(input.mouse_state().is_button_up(MouseButton::Right));
    REQUIRE(input.mouse_state().x() == 1);   // a button event carries the position

    input.update();
    REQUIRE_FALSE(input.mouse_state().is_button_pressed(MouseButton::Left));

    input.report_mouse_button(MouseButton::Left, false, {1, 2});
    input.update();
    REQUIRE(input.mouse_state().is_button_up(MouseButton::Left));
    REQUIRE(input.mouse_state().is_button_released(MouseButton::Left));
    input.update();
    REQUIRE_FALSE(input.mouse_state().is_button_released(MouseButton::Left));
}

TEST_CASE("mouse button numbers of the event map to the TS buttons") {
    REQUIRE(ris::mouse_button_from_event_button(1) == MouseButton::Left);
    REQUIRE(ris::mouse_button_from_event_button(2) == MouseButton::Middle);
    REQUIRE(ris::mouse_button_from_event_button(3) == MouseButton::Right);
    REQUIRE(ris::mouse_button_from_event_button(4) == MouseButton::X1);
    REQUIRE(ris::mouse_button_from_event_button(5) == MouseButton::X2);
    REQUIRE(ris::mouse_button_from_event_button(0) == MouseButton::None);
    REQUIRE(ris::mouse_button_from_event_button(99) == MouseButton::None);
}

TEST_CASE("losing focus drops held keys and buttons without reporting a release") {
    InputManager input;
    input.report_key(Key::A, true);
    input.report_mouse_button(MouseButton::Right, true, {0, 0});
    input.update();
    input.report_focus_lost();
    input.update();
    REQUIRE(input.keyboard_state().is_key_up(Key::A));
    REQUIRE_FALSE(input.keyboard_state().is_key_released(Key::A));
    REQUIRE(input.mouse_state().is_button_up(MouseButton::Right));
}

TEST_CASE("handle_event feeds the neutral Event of the window") {
    InputManager input;
    ris::Event ev;
    ev.type = ris::EventType::KeyDown;
    ev.scancode = static_cast<int32_t>(Key::W);
    input.handle_event(ev);
    ev = {};
    ev.type = ris::EventType::MouseMove;
    ev.x = 5; ev.y = 6; ev.dx = 2; ev.dy = 3;
    input.handle_event(ev);
    ev = {};
    ev.type = ris::EventType::MouseWheel;
    ev.dy = 1;
    input.handle_event(ev);
    input.update();
    REQUIRE(input.keyboard_state().is_key_down(Key::W));
    REQUIRE(input.mouse_state().x() == 5);
    REQUIRE(input.mouse_state().dy() == 3);
    REQUIRE(input.mouse_state().scroll_wheel_position.y == 1);
}

TEST_CASE("gamepads: not connected by default, out of range indices are safe") {
    InputManager input;
    input.update();
    REQUIRE_FALSE(input.game_pad_state().is_connected());
    REQUIRE_FALSE(input.game_pad_state(3).is_connected());
    REQUIRE_FALSE(input.game_pad_state(-1).is_connected());
    REQUIRE_FALSE(input.game_pad_state(99).is_connected());
    REQUIRE_FALSE(input.game_pad_state(99).is_button_down(GamePadButton::A));
    input.report_game_pad_button(99, GamePadButton::A, true);   // ignored
    input.update();
    REQUIRE_FALSE(input.game_pad_state(99).is_button_down(GamePadButton::A));
}

TEST_CASE("gamepad buttons, triggers and sticks") {
    InputManager input;
    input.report_game_pad_connected(1, true);
    input.report_game_pad_button(1, GamePadButton::A, true);
    input.report_game_pad_axes(1, {0.0f, 1.0f}, {-1.0f, 0.0f}, 0.5f, 1.0f);
    input.update();
    const ris::GamePadState& pad = input.game_pad_state(1);
    REQUIRE(pad.is_connected());
    REQUIRE(pad.is_button_down(GamePadButton::A));
    REQUIRE(pad.is_button_pressed(GamePadButton::A));
    REQUIRE(pad.is_button_up(GamePadButton::B));
    REQUIRE(pad.thumb_sticks().left.y == 1.0f);
    REQUIRE(pad.thumb_sticks().right.x == -1.0f);
    REQUIRE_THAT(pad.left_trigger(), Catch::Matchers::WithinAbs(0.5, 1e-6));
    REQUIRE(pad.right_trigger() == 1.0f);
    REQUIRE_FALSE(input.game_pad_state(0).is_connected());

    input.update();
    REQUIRE_FALSE(input.game_pad_state(1).is_button_pressed(GamePadButton::A));
    input.report_game_pad_button(1, GamePadButton::A, false);
    input.update();
    REQUIRE(input.game_pad_state(1).is_button_released(GamePadButton::A));
    REQUIRE(input.game_pad_state(1).is_button_up(GamePadButton::A));
}

TEST_CASE("the thumb stick dead zone is radial and does not rescale the output") {
    InputManager input;
    REQUIRE_THAT(input.thumbstick_dead_zone(), Catch::Matchers::WithinAbs(0.2, 1e-6));
    input.report_game_pad_connected(0, true);

    input.report_game_pad_axes(0, {0.1f, 0.1f}, {0.0f, 0.15f}, 0, 0);   // both inside the dead zone
    input.update();
    REQUIRE(input.game_pad_state().thumb_sticks().left == ris::Vec2(0, 0));
    REQUIRE(input.game_pad_state().thumb_sticks().right == ris::Vec2(0, 0));

    input.report_game_pad_axes(0, {1.0f, 0.0f}, {0.0f, -0.6f}, 0, 0);
    input.update();
    REQUIRE_THAT(input.game_pad_state().thumb_sticks().left.x, Catch::Matchers::WithinAbs(1.0, 1e-5));
    REQUIRE_THAT(input.game_pad_state().thumb_sticks().right.y, Catch::Matchers::WithinAbs(-0.6, 1e-5));   // outside the dead zone: unchanged

    input.set_thumbstick_dead_zone(0.0f);
    input.report_game_pad_axes(0, {0.1f, 0.0f}, {0, 0}, 0, 0);
    input.update();
    REQUIRE_THAT(input.game_pad_state().thumb_sticks().left.x, Catch::Matchers::WithinAbs(0.1, 1e-5));
}

TEST_CASE("unplugging a gamepad releases what it held and centres it") {
    InputManager input;
    input.report_game_pad_connected(0, true);
    input.report_game_pad_button(0, GamePadButton::X, true);
    input.report_game_pad_axes(0, {1, 0}, {0, 0}, 1, 0);
    input.update();
    REQUIRE(input.game_pad_state().is_button_down(GamePadButton::X));

    input.report_game_pad_connected(0, false);
    input.update();
    REQUIRE_FALSE(input.game_pad_state().is_connected());
    REQUIRE(input.game_pad_state().is_button_up(GamePadButton::X));
    REQUIRE(input.game_pad_state().is_button_released(GamePadButton::X));
    REQUIRE(input.game_pad_state().thumb_sticks().left == ris::Vec2(0, 0));
    REQUIRE(input.game_pad_state().left_trigger() == 0.0f);
}
