#include "ris/time/stopwatch.hpp"

namespace ris {

Stopwatch Stopwatch::start_new(const AClock& clock) noexcept {
    Stopwatch stopwatch(clock);
    stopwatch.start();
    return stopwatch;
}

void Stopwatch::start() noexcept {
    if (running_) {
        return;
    }
    started_at_ticks_ = clock_->ticks();
    running_ = true;
}

void Stopwatch::stop() noexcept {
    if (!running_) {
        return;
    }
    accumulated_ticks_ = elapsed_ticks();
    running_ = false;
}

void Stopwatch::reset() noexcept {
    accumulated_ticks_ = 0;
    running_ = false;
}

void Stopwatch::restart() noexcept {
    accumulated_ticks_ = 0;
    running_ = false;
    start();
}

uint64_t Stopwatch::elapsed_ticks() const noexcept {
    return running_ ? accumulated_ticks_ + (clock_->ticks() - started_at_ticks_) : accumulated_ticks_;
}

uint64_t Stopwatch::elapsed_ns() const noexcept { return clock_->ticks_to_ns(elapsed_ticks()); }

double Stopwatch::elapsed_ms() const noexcept { return static_cast<double>(elapsed_ns()) * 1e-6; }

double Stopwatch::elapsed_seconds() const noexcept { return static_cast<double>(elapsed_ns()) * 1e-9; }

} // namespace ris
