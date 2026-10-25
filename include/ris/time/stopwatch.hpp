#pragma once

#include <cstdint>

#include "ris/core.hpp"
#include "ris/time/a_clock.hpp"

namespace ris {

/// Measures real (clock) time, like `System.Diagnostics.Stopwatch`: start it, stop it, read the elapsed time. It
/// accumulates over several start / stop pairs until `reset()`. It reads an `AClock` (SDL3: the performance
/// counter), so it is as precise as the clock and unaffected by wall clock changes. Not thread safe; the clock
/// must outlive it.
class RIS_API Stopwatch {
public:
    /// Creates a stopped stopwatch at 0.
    explicit Stopwatch(const AClock& clock) noexcept : clock_(&clock) {}

    /// Creates a stopwatch that is already running.
    static Stopwatch start_new(const AClock& clock) noexcept;

    /// Starts or resumes measuring. Does nothing when it is already running.
    void start() noexcept;
    /// Stops measuring and keeps the elapsed time. Does nothing when it is stopped.
    void stop() noexcept;
    /// Stops the stopwatch and sets the elapsed time to 0.
    void reset() noexcept;
    /// Sets the elapsed time to 0 and starts the stopwatch.
    void restart() noexcept;

    /// True between `start()` and `stop()`.
    bool is_running() const noexcept { return running_; }
    /// Elapsed time in nanoseconds (up to now while running).
    uint64_t elapsed_ns() const noexcept;
    /// Elapsed time in milliseconds, with the fraction.
    double elapsed_ms() const noexcept;
    /// Elapsed time in seconds, with the fraction.
    double elapsed_seconds() const noexcept;

private:
    uint64_t elapsed_ticks() const noexcept;

    const AClock* clock_;
    uint64_t accumulated_ticks_ = 0;
    uint64_t started_at_ticks_ = 0;
    bool running_ = false;
};

} // namespace ris
