#pragma once

#include <cstdint>

namespace ris {

/// The state of a touch location for the current frame, like `TouchLocationState` of ris-framework-ts.
enum class TouchLocationState : int32_t {
    Invalid = 0,    ///< no contact
    Released = 1,   ///< the contact was released this frame
    Pressed = 2,    ///< the contact started this frame
    Moved = 3,      ///< the contact moved while held
    Cancelled = 4   ///< the contact was cancelled this frame. This is not a finger lift.
};

} // namespace ris
