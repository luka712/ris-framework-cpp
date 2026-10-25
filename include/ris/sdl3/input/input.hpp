#pragma once

// The SDL3 implementation of the input manager, the counterpart of the ris-framework-ts browser `InputManager`:
// `ris::sdl3::Sdl3InputManager` derives from `ris::InputManagerBase` (ris/input/) and translates SDL keyboard,
// mouse, gamepad and touch events. No SDL header is needed to use it.
//
// Other windowing / input APIs get their own folder next to this one (`ris/<api>/input/`) with a class derived
// from `InputManagerBase`, selected in `ris::create_default_input_manager()`.

#include "ris/sdl3/input/sdl3_input_manager.hpp"
