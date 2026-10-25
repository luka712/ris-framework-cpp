#pragma once

#include "ris/math.hpp"

namespace ris {

/// The thumbsticks of a gamepad, like `GamePadThumbSticks` of ris-framework-ts. Each stick is a vector with both
/// axes in [-1, 1]: x is positive to the right and **y is positive up** (the opposite of the screen / mouse
/// convention). Values inside the dead zone (`AInputManager::thumbstick_dead_zone`) are reported as (0, 0).
struct GamePadThumbSticks {
    Vec2 left{0, 0};
    Vec2 right{0, 0};
};

} // namespace ris
