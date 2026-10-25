#pragma once

#include <array>
#include <cstdint>

#include "core.hpp"
#include "math.hpp"
#include "window.hpp"

namespace ris {

/// Physical keys. The values are SDL scancodes (USB HID usages), the same as `Event::scancode`.
enum class Key : int32_t {
    A = 4, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num1 = 30, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Num0,
    Enter = 40, Escape = 41, Backspace = 42, Tab = 43, Space = 44,
    F1 = 58, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Right = 79, Left = 80, Down = 81, Up = 82,
    LeftCtrl = 224, LeftShift = 225, LeftAlt = 226, RightCtrl = 228, RightShift = 229, RightAlt = 230
};

enum class MouseButton : int32_t { Left = 1, Middle = 2, Right = 3 };

/// Snapshot of the keyboard for one frame.
class KeyboardState {
public:
    bool is_key_down(Key key) const { return get(down_, key); }
    bool is_key_up(Key key) const { return !is_key_down(key); }
    /// True on the frame the key went from down to up.
    bool is_key_released(Key key) const { return get(released_, key); }

private:
    friend class InputManager;
    static constexpr size_t kKeys = 512;
    static bool get(const std::array<bool, kKeys>& a, Key k) {
        const auto i = static_cast<size_t>(k);
        return i < kKeys && a[i];
    }
    std::array<bool, kKeys> down_{};
    std::array<bool, kKeys> released_{};
};

/// Snapshot of the mouse for one frame. Positions are in window pixels, y pointing down.
struct MouseState {
    Vec2 position{0, 0};
    Vec2 delta{0, 0};                 ///< movement during the frame
    Vec2 scroll_wheel_position{0, 0}; ///< accumulated wheel movement since start
    Vec2 scroll_wheel_delta{0, 0};    ///< wheel movement during the frame

    float x() const { return position.x; }
    float y() const { return position.y; }
    float dx() const { return delta.x; }
    float dy() const { return delta.y; }

    bool is_button_down(MouseButton b) const { return (down_ & bit(b)) != 0; }
    bool is_button_up(MouseButton b) const { return !is_button_down(b); }
    bool is_button_released(MouseButton b) const { return (released_ & bit(b)) != 0; }

private:
    friend class InputManager;
    static uint32_t bit(MouseButton b) { return 1u << (static_cast<uint32_t>(b) & 31u); }
    uint32_t down_ = 0, released_ = 0;
};

/// Collects window events and exposes per frame keyboard and mouse state, like `IInputManager` of
/// ris-framework-ts. Gamepad and touch input are not implemented in 0.1.0.
class RIS_API InputManager {
public:
    const KeyboardState& keyboard_state() const { return keyboard_; }
    const MouseState& mouse_state() const { return mouse_; }

    /// Called by the framework for every event. Public so applications that run their own loop can feed it.
    void handle_event(const Event& event);
    /// Called by the framework once per frame, before the update listeners: publishes the events
    /// collected since the last call.
    void begin_frame();

private:
    KeyboardState keyboard_, pending_keyboard_;
    MouseState mouse_, pending_mouse_;
    Vec2 pending_delta_{0, 0}, pending_wheel_{0, 0};
};

} // namespace ris
