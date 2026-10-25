#pragma once

#include <cstdint>

namespace ris {

/// The buttons of a gamepad, like `Button` of ris-framework-ts (same names and values). The face buttons use the
/// Xbox layout: `A` is the bottom button, `B` right, `X` left and `Y` top, whatever the controller prints on them.
enum class GamePadButton : int32_t {
    None = 0,
    A = 1,
    B = 2,
    Back = 3,
    BigButton = 4,   ///< the guide / home button
    DPadDown = 5,
    DPadLeft = 6,
    DPadRight = 7,
    DPadUp = 8,
    LeftShoulder = 9,
    LeftStick = 10,  ///< left stick pressed in
    RightShoulder = 11,
    RightStick = 12, ///< right stick pressed in
    Start = 13,
    X = 14,
    Y = 15
};

/// Number of button slots tracked: every GamePadButton value is smaller than this.
inline constexpr int32_t game_pad_button_count = 16;

} // namespace ris
