#pragma once

#include <cstdint>

#include "ris/input/mouse/mouse_button.hpp"
#include "ris/math.hpp"

namespace ris {

class InputManager;

/// Snapshot of the mouse for one frame, like `MouseState` of ris-framework-ts.
///
/// Positions are in window coordinates (the unit the windowing system reports, y pointing down, origin at the top
/// left of the window). The input manager publishes a new snapshot in `InputManager::update()`; the reference
/// returned by `mouse_state()` stays valid for the lifetime of the manager.
///
/// Differences from TS: the wheel is reported as the movement of the frame and as a running total instead of a
/// -1 / 0 / 1 direction, and there is `is_button_pressed`.
struct MouseState {
    Vec2 position{0, 0};                ///< cursor position
    Vec2 delta{0, 0};                   ///< cursor movement during the frame
    Vec2 scroll_wheel_position{0, 0};   ///< accumulated wheel movement since start
    Vec2 scroll_wheel_delta{0, 0};      ///< wheel movement during the frame

    float x() const noexcept { return position.x; }
    float y() const noexcept { return position.y; }
    float dx() const noexcept { return delta.x; }
    float dy() const noexcept { return delta.y; }

    /// True while the button is held down in this frame.
    bool is_button_down(MouseButton button) const noexcept { return (down_ & bit(button)) != 0; }
    /// True while the button is not held down in this frame.
    bool is_button_up(MouseButton button) const noexcept { return !is_button_down(button); }
    /// True in the frame in which the button went from up to down.
    bool is_button_pressed(MouseButton button) const noexcept { return (pressed_ & bit(button)) != 0; }
    /// True in the frame in which the button went from down to up.
    bool is_button_released(MouseButton button) const noexcept { return (released_ & bit(button)) != 0; }

private:
    friend class InputManager;
    /// The bit of a button, 0 for `None` and for values outside the known buttons.
    static constexpr uint32_t bit(MouseButton button) noexcept {
        const auto i = static_cast<int32_t>(button);
        return (i > 0 && i < mouse_button_count) ? (1u << static_cast<uint32_t>(i)) : 0u;
    }
    uint32_t down_ = 0, pressed_ = 0, released_ = 0;
};

} // namespace ris
