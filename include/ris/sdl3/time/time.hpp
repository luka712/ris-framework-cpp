#pragma once

// The SDL3 implementation of the time layer: `ris::sdl3::Sdl3Clock` (the SDL performance counter, delays and
// wall clock behind `ris::AClock`) and `ris::sdl3::Sdl3TimeManager` (`ris::TimeManagerBase` on that clock). No SDL
// header is needed to use them.
//
// Other platform APIs get their own folder next to this one (`ris/<api>/time/`) with an `AClock`, selected in
// `ris::default_clock()` and `ris::create_default_time_manager()`.

#include "ris/sdl3/time/sdl3_clock.hpp"
#include "ris/sdl3/time/sdl3_time_manager.hpp"
