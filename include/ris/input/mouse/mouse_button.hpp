#pragma once

#include <cstdint>

namespace ris {

/// The mouse buttons, like `MouseButton` of ris-framework-ts (same values: `None` 0, `Left` 1, `Right` 2,
/// `Middle` 3), plus the two extra buttons.
///
/// Note that `Event::button` and `ris_event.button` use the SDL numbering (1 left, 2 middle, 3 right, 4 and 5
/// extra); convert it with `mouse_button_from_event_button`.
enum class MouseButton : int32_t {
    None = 0,
    Left = 1,
    Right = 2,
    Middle = 3,
    X1 = 4,   ///< first extra button, usually "back"
    X2 = 5    ///< second extra button, usually "forward"
};

/// Number of mouse button slots tracked: every MouseButton value is smaller than this.
inline constexpr int32_t mouse_button_count = 6;

/// Converts the button number of `Event` (1 left, 2 middle, 3 right, 4 X1, 5 X2) to a MouseButton.
/// Unknown numbers give `MouseButton::None`, which is never reported as down.
constexpr MouseButton mouse_button_from_event_button(int32_t button) noexcept {
    switch (button) {
        case 1: return MouseButton::Left;
        case 2: return MouseButton::Middle;
        case 3: return MouseButton::Right;
        case 4: return MouseButton::X1;
        case 5: return MouseButton::X2;
        default: return MouseButton::None;
    }
}

} // namespace ris
