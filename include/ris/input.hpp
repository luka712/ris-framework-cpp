#pragma once

// Umbrella header of ris/input/, mirroring the ris-framework-ts `api/input` folder: the concrete `InputManager`
// (`IInputManager` / `InputManager`), the state classes `KeyboardState`, `MouseState`, `GamePadState`
// (+ `GamePadThumbSticks`), `TouchCollection` / `TouchLocation` / `TouchLocationState` / `TouchPanelCapabilities`,
// and the enums `Key`, `MouseButton`, `GamePadButton` (`Button`) and `InputDeviceType`. Each lives in its own header
// of ris/input/ (mouse/, gamepad/, touch/ like TS); this header includes all of them.
//
// SDL3 is an integral part of the engine (windowing, input, time), so there is no abstract input manager and no
// backend split here: `InputManager` uses SDL3 directly (keyboard, mouse, gamepads, touch). SDL types are never
// exposed: the SDL headers stay private to the library. Get the manager from `Framework::input_manager()`.

#include "ris/input/gamepad/game_pad_button.hpp"
#include "ris/input/gamepad/game_pad_state.hpp"
#include "ris/input/gamepad/game_pad_thumb_sticks.hpp"
#include "ris/input/input_device_type.hpp"
#include "ris/input/input_manager.hpp"
#include "ris/input/key.hpp"
#include "ris/input/keyboard_state.hpp"
#include "ris/input/mouse/mouse_button.hpp"
#include "ris/input/mouse/mouse_state.hpp"
#include "ris/input/touch/touch_collection.hpp"
#include "ris/input/touch/touch_location.hpp"
#include "ris/input/touch/touch_location_state.hpp"
#include "ris/input/touch/touch_panel_capabilities.hpp"
