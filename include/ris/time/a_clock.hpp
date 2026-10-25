#pragma once

#include <cstdint>
#include <optional>

#include "ris/core.hpp"
#include "ris/time/date_time.hpp"

namespace ris {

/// A source of time: a monotonic high resolution counter, delays and the wall clock. This is the one abstract
/// class that a platform backend implements to plug into the time layer (the SDL3 one is
/// `ris::sdl3::Sdl3Clock`); `TimeManagerBase`, `Stopwatch` and the rest of `ris/time/` only talk to it, so they
/// can be run against a fake clock without any platform API. It names no SDL (or other platform) types.
///
/// Thread rules: `ticks()`, `ticks_per_second()` and `unix_time_ns()` must be safe to call from any thread. The
/// delay functions block only the calling thread. A clock must outlive everything that was given a reference to it.
class RIS_API AClock {
public:
    virtual ~AClock() = default;

    /// The current value of the monotonic counter, in units of `1 / ticks_per_second()` seconds. It never goes
    /// backwards and is not related to any calendar time; only differences are meaningful. The origin is
    /// unspecified (often the system start), so do not assume it starts at 0.
    virtual uint64_t ticks() const noexcept = 0;
    /// The frequency of `ticks()`, in ticks per second. Constant for the life of the clock and greater than 0.
    virtual uint64_t ticks_per_second() const noexcept = 0;

    /// Sleeps the calling thread for about `ns` nanoseconds (coarse): at least `ns`, but the operating system may
    /// oversleep by its scheduling granularity. Cheap on the CPU. 0 returns at once.
    virtual void delay_ns(uint64_t ns) = 0;
    /// Sleeps the calling thread for `ns` nanoseconds as exactly as the platform can: it sleeps for most of the
    /// time and spins for the rest, so it costs some CPU. Use it for frame limiting. 0 returns at once.
    virtual void delay_precise_ns(uint64_t ns) = 0;

    /// The wall clock: nanoseconds since 1 January 1970 00:00:00 UTC (the Unix epoch), 0 if it cannot be read.
    /// Not monotonic: the user or the network may set it, so never use it to measure durations.
    virtual int64_t unix_time_ns() const noexcept = 0;
    /// Converts a time since the Unix epoch to a calendar date and time.
    /// @param unix_time_ns nanoseconds since the Unix epoch (UTC)
    /// @param local true: express it in the time zone of the system (`utc_offset_seconds` of the result is the
    ///        zone's offset at that time, daylight saving included); false: in UTC
    /// @return the date and time, or empty if the value cannot be represented
    virtual std::optional<DateTime> to_date_time(int64_t unix_time_ns, bool local) const = 0;
    /// Converts a calendar date and time to nanoseconds since the Unix epoch (UTC). The `utc_offset_seconds` field
    /// says which zone the fields are in; `day_of_week` is ignored.
    /// @return the time, or empty if a field is out of range or the result cannot be represented
    virtual std::optional<int64_t> from_date_time(const DateTime& date_time) const = 0;

    /// `ticks()` in nanoseconds, since the unspecified origin of the counter.
    uint64_t now_ns() const noexcept { return ticks_to_ns(ticks()); }
    /// `ticks()` in seconds, since the unspecified origin of the counter.
    double now_seconds() const noexcept { return static_cast<double>(now_ns()) * 1e-9; }
    /// The current wall clock time as a calendar date and time (`to_date_time(unix_time_ns(), local)`).
    std::optional<DateTime> current_date_time(bool local) const { return to_date_time(unix_time_ns(), local); }

    /// Converts a number of ticks (a duration, or a counter value) to nanoseconds, without overflow for any
    /// 64 bit counter at frequencies up to 18 GHz.
    uint64_t ticks_to_ns(uint64_t tick_count) const noexcept;
    /// Converts a duration in nanoseconds to ticks, rounding down.
    uint64_t ns_to_ticks(uint64_t ns) const noexcept;

    AClock() = default;
    AClock(const AClock&) = delete;
    AClock& operator=(const AClock&) = delete;
};

} // namespace ris
