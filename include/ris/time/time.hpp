#pragma once

// Platform API neutral time, mirroring the ris-framework-ts `api/time` folder (`GameTime`, `ITimeManager`,
// `TimeManager`): `GameTime`, the abstract `ATimeManager` (`ITimeManager`) and the reusable `TimeManagerBase` (the
// logic of `TimeManager`: delta, elapsed time, time scale, pause, frame limiting, fixed time step, frame rate), the
// abstract `AClock` (the platform time source: monotonic counter, delays, wall clock), the value type `DateTime`
// and the helpers `Stopwatch`, `Timer` and `FrameRateCounter`. Each lives in its own header of this folder; this
// header includes all of them.
//
// Nothing here depends on SDL or any other platform API. The SDL3 implementation is `ris::sdl3::Sdl3Clock` and
// `ris::sdl3::Sdl3TimeManager` in ris/sdl3/time/. Get the manager of the default backend from
// `Framework::time_manager()` or `create_default_time_manager()`. To add a backend, implement `AClock` and give it
// to `TimeManagerBase`.

#include "ris/time/a_clock.hpp"
#include "ris/time/a_time_manager.hpp"
#include "ris/time/date_time.hpp"
#include "ris/time/default_time_manager.hpp"
#include "ris/time/frame_rate_counter.hpp"
#include "ris/time/game_time.hpp"
#include "ris/time/stopwatch.hpp"
#include "ris/time/time_manager_base.hpp"
#include "ris/time/time_units.hpp"
#include "ris/time/timer.hpp"
