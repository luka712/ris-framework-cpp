#pragma once

#include <memory>

#include "ris/core.hpp"
#include "ris/time/a_clock.hpp"
#include "ris/time/a_time_manager.hpp"

namespace ris {

/// The one place that selects the platform time API: the clock the framework is built for, currently SDL3
/// (`ris::sdl3::Sdl3Clock`). The clock is stateless and thread safe, so every caller shares one instance, which
/// lives until the program ends. Usable before any window exists.
///
/// A future backend is added by implementing `AClock` in `ris/<api>/time/` and returning it from this function
/// (src/time/default_time_manager.cpp).
RIS_API AClock& default_clock();

/// Creates a time manager for the default backend (`ris::sdl3::Sdl3TimeManager`, on `default_clock()`). It is
/// ready to use; call `prepare_start()` before the first `frame_start()` (`Framework` does).
RIS_API std::unique_ptr<ATimeManager> create_default_time_manager();

} // namespace ris
