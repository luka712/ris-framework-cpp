#pragma once

// A backend without any windowing API: shows how another backend plugs in (derive from InputManagerBase, call the
// protected report_* functions from its own event source) and lets the tests drive the shared state tracking.
#include <ris/input/input.hpp>

namespace ris_test {

class FakeInputManager final : public ris::InputManagerBase {
public:
    void initialize() override { initialized = true; }
    void dispose() override { initialized = false; }

    using InputManagerBase::report_focus_lost;
    using InputManagerBase::report_game_pad_axes;
    using InputManagerBase::report_game_pad_button;
    using InputManagerBase::report_game_pad_connected;
    using InputManagerBase::report_key;
    using InputManagerBase::report_mouse_button;
    using InputManagerBase::report_mouse_move;
    using InputManagerBase::report_mouse_wheel;
    using InputManagerBase::report_touch;
    using InputManagerBase::set_touch_capabilities;

    bool initialized = false;
};

} // namespace ris_test
