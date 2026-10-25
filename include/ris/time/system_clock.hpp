#pragma once

#include <cstdint>
#include <optional>

#include "ris/core.hpp"
#include "ris/time/date_time.hpp"

/// The system clocks of the platform, through SDL3 (which is an integral part of the engine, not an optional
/// backend). Free functions, all stateless and safe to call from any thread, and none of them needs SDL to be
/// initialised or a window to exist. No SDL type appears in this header.
///
/// Mapping:
///  - `ticks()` / `ticks_per_second()`: `SDL_GetPerformanceCounter()` / `SDL_GetPerformanceFrequency()` (the
///    monotonic high resolution clock of the platform; 1 GHz on Linux, QueryPerformanceCounter on Windows);
///  - `delay_ns()`: `SDL_DelayNS()`; `delay_precise_ns()`: `SDL_DelayPrecise()` (sleeps most of the time, then spins);
///  - `unix_time_ns()`: `SDL_GetCurrentTime()`; `to_date_time()` / `from_date_time()`: `SDL_TimeToDateTime()` /
///    `SDL_DateTimeToTime()`. `SDL_DateTimeToTime()` honours `utc_offset_seconds`; the local time zone is the one
///    SDL / the C library report.
namespace ris::system_clock {

/// The current value of the monotonic counter, in units of `1 / ticks_per_second()` seconds. Never goes backwards;
/// only differences are meaningful.
RIS_API uint64_t ticks() noexcept;
/// The frequency of `ticks()`, in ticks per second. Constant and greater than 0.
RIS_API uint64_t ticks_per_second() noexcept;
/// `ticks()` in nanoseconds, since the unspecified origin of the counter.
RIS_API uint64_t now_ns() noexcept;
/// `ticks()` in seconds, since the unspecified origin of the counter.
RIS_API double now_seconds() noexcept;

/// Sleeps the calling thread for about `ns` nanoseconds (coarse): at least `ns`, but the operating system may
/// oversleep by its scheduling granularity. Cheap on the CPU. 0 returns at once.
RIS_API void delay_ns(uint64_t ns) noexcept;
/// Sleeps the calling thread for `ns` nanoseconds as exactly as the platform can: it sleeps for most of the time and
/// spins for the rest, so it costs some CPU. 0 returns at once.
RIS_API void delay_precise_ns(uint64_t ns) noexcept;

/// The wall clock: nanoseconds since 1 January 1970 00:00:00 UTC (the Unix epoch), 0 if it cannot be read. Not
/// monotonic: the user or the network may set it, so never use it to measure durations.
RIS_API int64_t unix_time_ns() noexcept;
/// Converts a time since the Unix epoch to a calendar date and time.
/// @param unix_time_ns nanoseconds since the Unix epoch (UTC)
/// @param local true: express it in the time zone of the system (`utc_offset_seconds` of the result is the zone's
///        offset at that time, daylight saving included); false: in UTC
/// @return the date and time, or empty if the value cannot be represented
RIS_API std::optional<DateTime> to_date_time(int64_t unix_time_ns, bool local) noexcept;
/// Converts a calendar date and time to nanoseconds since the Unix epoch (UTC). The `utc_offset_seconds` field says
/// which zone the fields are in; `day_of_week` is ignored.
/// @return the time, or empty if a field is out of range or the result cannot be represented
RIS_API std::optional<int64_t> from_date_time(const DateTime& date_time) noexcept;
/// The current wall clock time as a calendar date and time (`to_date_time(unix_time_ns(), local)`).
RIS_API std::optional<DateTime> current_date_time(bool local) noexcept;

} // namespace ris::system_clock
