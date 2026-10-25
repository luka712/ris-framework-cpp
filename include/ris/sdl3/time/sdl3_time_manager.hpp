#pragma once

#include "ris/core.hpp"
#include "ris/time/time_manager_base.hpp"

namespace ris::sdl3 {

/// The SDL3 implementation of the time manager (`ris::ATimeManager`): `ris::TimeManagerBase` driven by the
/// shared `Sdl3Clock::shared()` (`Sdl3Clock`: SDL performance counter, SDL delays). All the logic (delta, elapsed,
/// time scale, pause, fixed step, frame limiting, frame rate) is in the base class and is the same for every
/// backend; this class only picks the clock. No SDL type appears in this header. Not thread safe: use it on the
/// thread that runs the frame loop.
class RIS_API Sdl3TimeManager final : public TimeManagerBase {
public:
    Sdl3TimeManager();
};

} // namespace ris::sdl3
