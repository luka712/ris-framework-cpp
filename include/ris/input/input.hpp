#pragma once

// Windowing API neutral input, mirroring the ris-framework-ts `api/input` folder: the abstract `AInputManager`
// (`IInputManager`), the state classes `KeyboardState`, `MouseState`, `GamePadState` (+ `GamePadThumbSticks`),
// `TouchCollection` / `TouchLocation` / `TouchLocationState` / `TouchPanelCapabilities`, and the enums `Key`,
// `MouseButton`, `GamePadButton` (`Button`) and `InputDeviceType`. Each lives in its own header of this folder
// (mouse/, gamepad/, touch/ like TS); this header includes all of them.
//
// Nothing here depends on SDL or any other platform API. `InputManagerBase` is the shared state tracking every
// backend derives from; the SDL3 implementation is `ris::sdl3::Sdl3InputManager` in ris/sdl3/input/. Get the
// manager of the default backend from `Framework::input_manager()` or `create_default_input_manager()`. To add a
// backend, derive from `InputManagerBase`, translate native events to its `report_*` functions and implement
// `initialize()` and `dispose()`.

#include "ris/input/a_input_manager.hpp"
#include "ris/input/default_input_manager.hpp"
#include "ris/input/gamepad/game_pad_button.hpp"
#include "ris/input/gamepad/game_pad_state.hpp"
#include "ris/input/gamepad/game_pad_thumb_sticks.hpp"
#include "ris/input/input_device_type.hpp"
#include "ris/input/input_manager_base.hpp"
#include "ris/input/key.hpp"
#include "ris/input/keyboard_state.hpp"
#include "ris/input/mouse/mouse_button.hpp"
#include "ris/input/mouse/mouse_state.hpp"
#include "ris/input/touch/touch_collection.hpp"
#include "ris/input/touch/touch_location.hpp"
#include "ris/input/touch/touch_location_state.hpp"
#include "ris/input/touch/touch_panel_capabilities.hpp"
