#pragma once

#include "core.hpp"

namespace ris {

/// Time information passed to update listeners. Mirrors `GameTime` of ris-framework-ts.
struct GameTime {
    double elapsed_time_ms = 0;   ///< since the framework started the loop
    double delta_time_ms = 0;     ///< since the previous frame

    float elapsed_time_sec() const { return static_cast<float>(elapsed_time_ms / 1000.0); }
    float delta_time_sec() const { return static_cast<float>(delta_time_ms / 1000.0); }
};

class RIS_API TimeManager {
public:
    /// `fixed_step_ms` > 0 replaces the real clock with a fixed step, for reproducible runs.
    explicit TimeManager(double fixed_step_ms = 0) : fixed_step_ms_(fixed_step_ms) {}

    /// Called by the framework right before the first frame.
    void prepare_start();
    /// Called by the framework at the start of every frame.
    void frame_start();

    const GameTime& time() const { return time_; }

private:
    double fixed_step_ms_;
    double start_s_ = 0, last_s_ = 0;
    int64_t frame_ = 0;
    GameTime time_;
};

} // namespace ris
