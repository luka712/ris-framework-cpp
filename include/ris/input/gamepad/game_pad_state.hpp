#pragma once

#include <cstdint>

#include "ris/input/gamepad/game_pad_button.hpp"
#include "ris/input/gamepad/game_pad_thumb_sticks.hpp"

namespace ris {

class InputManagerBase;

/// Number of gamepads an input manager tracks (indices 0 to max_game_pad_count - 1).
inline constexpr int32_t max_game_pad_count = 4;

/// Snapshot of one gamepad for one frame, like `GamePadState` of ris-framework-ts.
///
/// Obtained from `AInputManager::game_pad_state(index)`. The reference stays valid for the lifetime of the manager
/// and always shows the latest published snapshot. A gamepad that is not connected (or an index that is out of
/// range) reports `is_connected() == false`, all buttons up and the sticks and triggers at zero, so code does not
/// have to check for devices first.
class GamePadState {
public:
    /// True while a gamepad is connected at this index.
    bool is_connected() const noexcept { return connected_; }
    /// The thumbsticks, dead zone applied.
    const GamePadThumbSticks& thumb_sticks() const noexcept { return sticks_; }
    /// The left trigger, 0 (released) to 1 (fully pressed). Not part of ris-framework-ts.
    float left_trigger() const noexcept { return left_trigger_; }
    /// The right trigger, 0 (released) to 1 (fully pressed). Not part of ris-framework-ts.
    float right_trigger() const noexcept { return right_trigger_; }

    /// True while the button is held down in this frame.
    bool is_button_down(GamePadButton button) const noexcept { return (down_ & bit(button)) != 0; }
    /// True while the button is not held down in this frame.
    bool is_button_up(GamePadButton button) const noexcept { return !is_button_down(button); }
    /// True in the frame in which the button went from up to down.
    bool is_button_pressed(GamePadButton button) const noexcept { return (pressed_ & bit(button)) != 0; }
    /// True in the frame in which the button went from down to up (also when the gamepad was unplugged).
    bool is_button_released(GamePadButton button) const noexcept { return (released_ & bit(button)) != 0; }

private:
    friend class InputManagerBase;
    static constexpr uint32_t bit(GamePadButton button) noexcept {
        const auto i = static_cast<int32_t>(button);
        return (i > 0 && i < game_pad_button_count) ? (1u << static_cast<uint32_t>(i)) : 0u;
    }
    bool connected_ = false;
    GamePadThumbSticks sticks_{};
    float left_trigger_ = 0, right_trigger_ = 0;
    uint32_t down_ = 0, pressed_ = 0, released_ = 0;
};

} // namespace ris
