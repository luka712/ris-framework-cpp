#include "ris/time/timer.hpp"

#include <cmath>

namespace ris {

Timer::Timer(double interval_ms, bool repeating, bool start_running)
    : interval_ms_(interval_ms), repeating_(repeating), running_(start_running) {
    if (!(interval_ms > 0.0) || std::isinf(interval_ms)) {
        throw Error(Result::InvalidArgument, "Timer interval must be a positive, finite number of milliseconds");
    }
}

void Timer::start() noexcept {
    if (!finished_) {
        running_ = true;
    }
}

void Timer::stop() noexcept { running_ = false; }

void Timer::reset() noexcept {
    elapsed_ms_ = 0;
    finished_ = false;
}

uint32_t Timer::update(double delta_ms) noexcept {
    if (!running_ || finished_ || !(delta_ms > 0.0)) {
        return 0;
    }
    elapsed_ms_ += delta_ms;
    if (elapsed_ms_ < interval_ms_) {
        return 0;
    }
    if (!repeating_) {
        elapsed_ms_ = interval_ms_;
        finished_ = true;
        running_ = false;
        return 1;
    }
    const double fired = std::floor(elapsed_ms_ / interval_ms_);
    elapsed_ms_ -= fired * interval_ms_;
    if (elapsed_ms_ < 0.0) {
        elapsed_ms_ = 0.0;
    }
    return fired >= 4294967295.0 ? 4294967295u : static_cast<uint32_t>(fired);
}

} // namespace ris
