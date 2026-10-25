#pragma once

#include <cstdint>

#include "ris/core.hpp"

namespace ris {

/// A countdown driven by game time: feed it `GameTime::delta_time_ms` every frame, so it obeys the time scale and
/// pause of the time manager, and it reports how often it elapsed. One shot (fires once, then stays finished) or
/// repeating (fires every interval). It runs on the thread that calls `update()`: no callbacks, no other thread,
/// so game state needs no locking. Not thread safe.
class RIS_API Timer {
public:
    /// @param interval_ms time until it fires, in milliseconds; must be > 0 (otherwise it throws
    ///        `std::invalid_argument`)
    /// @param repeating true: fires every `interval_ms`; false: fires once
    /// @param start_running true starts counting at once; false needs `start()`
    explicit Timer(double interval_ms, bool repeating = false, bool start_running = true);

    /// Starts counting, or resumes after `stop()`. Does nothing on a finished one shot timer (use `reset()`).
    void start() noexcept;
    /// Pauses counting and keeps the elapsed time.
    void stop() noexcept;
    /// Sets the elapsed time to 0 and clears `is_finished()`. The running state is kept, so a finished one shot
    /// timer (which stopped when it fired) needs `start()` again.
    void reset() noexcept;

    /// Advances the timer by `delta_ms` (negative and NaN values count as 0). A repeating timer that is advanced
    /// by several intervals at once (a long frame) fires several times.
    /// @return how many times the timer fired during this call: 0 or 1 for a one shot timer; for a repeating timer
    ///         0 or more
    uint32_t update(double delta_ms) noexcept;

    /// True while the timer counts (`start()`ed or created running, and not stopped or, for one shot, finished).
    bool is_running() const noexcept { return running_; }
    /// True once a one shot timer has fired; always false for a repeating timer.
    bool is_finished() const noexcept { return finished_; }
    /// True for a repeating timer.
    bool is_repeating() const noexcept { return repeating_; }
    /// The interval in milliseconds.
    double interval_ms() const noexcept { return interval_ms_; }
    /// Time counted towards the next firing, in milliseconds, 0 up to `interval_ms()`.
    double elapsed_ms() const noexcept { return elapsed_ms_; }
    /// Time until the next firing, in milliseconds.
    double remaining_ms() const noexcept { return interval_ms_ - elapsed_ms_; }
    /// `elapsed_ms() / interval_ms()`, 0 to 1 (1 for a finished one shot timer).
    double progress() const noexcept { return elapsed_ms_ / interval_ms_; }

private:
    double interval_ms_;
    double elapsed_ms_ = 0;
    bool repeating_;
    bool running_;
    bool finished_ = false;
};

} // namespace ris
