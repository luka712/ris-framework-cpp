#pragma once

#include <cstddef>
#include <array>

#include "ris/input/key.hpp"

namespace ris {

class InputManager;

/// Snapshot of the keyboard for one frame, like `KeyboardState` of ris-framework-ts.
///
/// The input manager publishes a new snapshot in `InputManager::update()`; the reference returned by
/// `keyboard_state()` stays valid for the lifetime of the manager and always shows the latest snapshot, so it can
/// be kept. Read it on the thread that runs the frame loop. A key that is outside [1, key_count) is never down.
class KeyboardState {
public:
    /// True while the key is held down in this frame.
    bool is_key_down(Key key) const noexcept { return get(down_, key); }
    /// True while the key is not held down in this frame.
    bool is_key_up(Key key) const noexcept { return !is_key_down(key); }
    /// True in the frame in which the key went from up to down (auto repeat does not count). A key that is
    /// pressed and released within one frame is reported as pressed and released, but not as down.
    bool is_key_pressed(Key key) const noexcept { return get(pressed_, key); }
    /// True in the frame in which the key went from down to up.
    bool is_key_released(Key key) const noexcept { return get(released_, key); }

private:
    friend class InputManager;
    using Flags = std::array<bool, static_cast<std::size_t>(key_count)>;
    static bool get(const Flags& flags, Key key) noexcept {
        const auto i = static_cast<int64_t>(static_cast<int32_t>(key));
        return i > 0 && i < key_count && flags[static_cast<std::size_t>(i)];
    }
    Flags down_{}, pressed_{}, released_{};
};

} // namespace ris
