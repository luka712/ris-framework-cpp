#include "ris/time/frame_rate_counter.hpp"

namespace ris {

FrameRateCounter::FrameRateCounter(double interval_ms) : interval_ms_(interval_ms > 0.0 ? interval_ms : 500.0) {}

void FrameRateCounter::add_frame(double frame_ms) noexcept {
    if (!(frame_ms > 0.0)) {   // also NaN
        frame_ms = 0.0;
    }
    window_ms_ += frame_ms;
    ++window_frames_;
    if (window_ms_ >= interval_ms_) {
        published_fps_ = static_cast<double>(window_frames_) * 1000.0 / window_ms_;
        has_published_ = true;
        window_ms_ = 0;
        window_frames_ = 0;
    }
}

void FrameRateCounter::reset() noexcept {
    window_ms_ = 0;
    window_frames_ = 0;
    published_fps_ = 0;
    has_published_ = false;
}

double FrameRateCounter::frames_per_second() const noexcept {
    if (has_published_) {
        return published_fps_;
    }
    return window_ms_ > 0.0 ? static_cast<double>(window_frames_) * 1000.0 / window_ms_ : 0.0;
}

} // namespace ris
