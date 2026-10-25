#include "ris/time.hpp"

#include <algorithm>

namespace ris {

void TimeManager::prepare_start() {
    start_s_ = last_s_ = time_seconds();
    frame_ = 0;
    time_ = {};
}

void TimeManager::frame_start() {
    ++frame_;
    if (fixed_step_ms_ > 0) {
        time_.delta_time_ms = fixed_step_ms_;
        time_.elapsed_time_ms = static_cast<double>(frame_ - 1) * fixed_step_ms_;
        return;
    }
    const double now = time_seconds();
    time_.delta_time_ms = (now - last_s_) * 1000.0;
    time_.elapsed_time_ms = (now - start_s_) * 1000.0;
    last_s_ = now;
}

} // namespace ris
