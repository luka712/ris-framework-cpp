#pragma once

#include <memory>

#include "ris/core.hpp"
#include "ris/input/a_input_manager.hpp"

namespace ris {

/// The one place that selects the windowing / input backend: creates the input manager of the backend the framework
/// is built for, currently SDL3 (`ris::sdl3::Sdl3InputManager`). The manager is not initialised yet: call
/// `initialize()` after the window exists (Framework does).
///
/// A future backend is added by deriving from `InputManagerBase` in `ris/<api>/input/` and returning it from this
/// function (src/input/default_input_manager.cpp).
RIS_API std::unique_ptr<AInputManager> create_default_input_manager();

} // namespace ris
