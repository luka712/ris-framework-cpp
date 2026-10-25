#pragma once

#include <cstdint>

#include "ris/core.hpp"

namespace ris {

/// Measures frames per second by counting frames over a fixed window of time. Pure arithmetic, no clock: feed it
/// the duration of each frame. Not thread safe.
class RIS_API FrameRateCounter {
public:
    /// @param interval_ms length of the measuring window in milliseconds; values <= 0 or NaN use 500
    explicit FrameRateCounter(double interval_ms = 500.0);

    /// Counts one frame that took `frame_ms` milliseconds (negative and NaN values count as 0). When the window is
    /// full the frame rate is published and a new window starts.
    void add_frame(double frame_ms) noexcept;
    /// Forgets all frames: the frame rate is 0 again.
    void reset() noexcept;

    /// Frames per second: the value of the last completed window; until the first window completes, the average
    /// over the frames counted so far. 0 when no time has been counted.
    double frames_per_second() const noexcept;
    /// The length of the measuring window in milliseconds.
    double interval_ms() const noexcept { return interval_ms_; }

private:
    double interval_ms_;
    double window_ms_ = 0;
    uint32_t window_frames_ = 0;
    double published_fps_ = 0;
    bool has_published_ = false;
};

} // namespace ris
