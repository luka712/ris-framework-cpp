#pragma once

#include <cstdint>
#include <functional>

#include "ris/core.hpp"

namespace ris {

/// The time source of `TimeManager` and `Stopwatch`: a monotonic high resolution counter and a precise delay,
/// given as plain functions. `ClockSource::system()` is the real one (the SDL3 performance counter and
/// `SDL_DelayPrecise`); the engine always uses it. The type exists as a test seam: a unit test builds a
/// `ClockSource` whose counter it moves by hand, so time can be driven without sleeping (see
/// tests/time/fake_clock.hpp). It names no SDL types, and the functions may be lambdas that capture state.
///
/// Rules for a custom source: `ticks` never goes backwards and is safe to call from the thread that uses the
/// manager; `ticks_per_second` is greater than 0 and constant; `delay_precise_ns` may be empty (then nothing waits).
struct ClockSource {
    /// The current value of the monotonic counter, in units of `1 / ticks_per_second` seconds. Only differences
    /// are meaningful; the origin is unspecified (often the system start), so do not assume it starts at 0.
    std::function<uint64_t()> ticks;
    /// The frequency of `ticks`, in ticks per second. Greater than 0.
    uint64_t ticks_per_second = 1'000'000'000;
    /// Blocks the calling thread for `ns` nanoseconds as exactly as the source can (the system one sleeps for most
    /// of the time and spins for the rest). `TimeManager::frame_end` uses it for frame limiting. May be empty.
    std::function<void(uint64_t ns)> delay_precise_ns;

    /// The real clock: `SDL_GetPerformanceCounter` / `SDL_GetPerformanceFrequency` and `SDL_DelayPrecise`. SDL does not
    /// need to be initialised. Every call returns an equal source that is safe to use from any thread.
    RIS_API static ClockSource system();

    /// Throws `Error(Result::InvalidArgument)` when `ticks` is empty or `ticks_per_second` is 0.
    RIS_API void validate() const;

    /// `ticks()` in nanoseconds, since the unspecified origin of the counter.
    RIS_API uint64_t now_ns() const;
    /// `ticks()` in seconds, since the unspecified origin of the counter.
    RIS_API double now_seconds() const;

    /// Converts a number of ticks (a duration, or a counter value) to nanoseconds, without overflow for any 64 bit
    /// counter at frequencies up to 18 GHz.
    RIS_API uint64_t ticks_to_ns(uint64_t tick_count) const noexcept;
    /// Converts a duration in nanoseconds to ticks, rounding down.
    RIS_API uint64_t ns_to_ticks(uint64_t ns) const noexcept;
};

} // namespace ris
