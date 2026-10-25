#pragma once

// Umbrella header of ris/time/, mirroring the ris-framework-ts `api/time` folder (`GameTime`, `ITimeManager` /
// `TimeManager`): `GameTime`, the `TimeManager` (delta, elapsed time, time scale, pause, frame limiting, fixed time
// step, frame rate), the `ClockSource` it measures with, the SDL3 `system_clock` functions (counter, delays, wall
// clock and date conversion), the value type `DateTime` and the helpers `Stopwatch`, `Timer` and
// `FrameRateCounter`. Each lives in its own header of ris/time/; this header includes all of them.
//
// SDL3 is an integral part of the engine (windowing, input, time), so there is no backend split here. SDL types
// are never exposed: the SDL headers stay private to the library.

#include "ris/time/clock_source.hpp"
#include "ris/time/date_time.hpp"
#include "ris/time/frame_rate_counter.hpp"
#include "ris/time/game_time.hpp"
#include "ris/time/stopwatch.hpp"
#include "ris/time/system_clock.hpp"
#include "ris/time/time_manager.hpp"
#include "ris/time/time_units.hpp"
#include "ris/time/timer.hpp"
