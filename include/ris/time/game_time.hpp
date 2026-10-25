#pragma once

#include <cstdint>

namespace ris {

/// The time information passed to the update listeners, like `GameTime` of ris-framework-ts (which has
/// `elapsedTimeMs` and `deltaTimeMs`). A plain value type, owned by the time manager and refreshed once per
/// frame; copy it if you need to keep a frame's values.
///
/// "Scaled" values follow the time manager's time scale and pause: they are what game logic should use. "Unscaled"
/// values are real (clock) time and keep running while the game is paused or slowed down.
struct GameTime {
    /// Scaled time since the loop started, in milliseconds. In real time mode it includes the current frame's
    /// delta; in deterministic mode (`TimeManager::set_deterministic_step_ms`) it is the time at the start of the
    /// frame, so the first frame is at 0.
    double elapsed_time_ms = 0;
    /// Scaled time since the previous frame, in milliseconds: the unscaled delta times the time scale, and 0
    /// while the time manager is paused.
    double delta_time_ms = 0;
    /// Unscaled (real) time since the loop started, in milliseconds. Same convention as `elapsed_time_ms`.
    double unscaled_elapsed_time_ms = 0;
    /// Unscaled (real) time since the previous frame, in milliseconds.
    double unscaled_delta_time_ms = 0;
    /// The unscaled delta, smoothed with an exponential moving average (see `TimeManager::set_delta_smoothing`).
    /// Equal to `unscaled_delta_time_ms` while smoothing is off (the default).
    double smoothed_delta_time_ms = 0;
    /// Frames per second, measured over the last half second of unscaled time; 0 before the first frame.
    double frames_per_second = 0;
    /// The time scale in effect for this frame (1 = real time).
    double time_scale = 1;
    /// Number of the current frame: 0 before the first `frame_start`, 1 during the first frame.
    uint64_t frame_number = 0;
    /// True while the time manager is paused: `delta_time_ms` is 0 and `elapsed_time_ms` stands still.
    bool is_paused = false;
    /// True when this frame took clearly longer than the target frame time (see
    /// `TimeManager::set_target_frames_per_second`), or when fixed steps had to be dropped to catch up (see
    /// `TimeManager::set_max_fixed_steps_per_frame`). Never set on the first frame.
    bool is_running_slowly = false;

    /// `elapsed_time_ms` in seconds.
    float elapsed_time_sec() const { return static_cast<float>(elapsed_time_ms / 1000.0); }
    /// `delta_time_ms` in seconds.
    float delta_time_sec() const { return static_cast<float>(delta_time_ms / 1000.0); }
    /// `unscaled_elapsed_time_ms` in seconds.
    float unscaled_elapsed_time_sec() const { return static_cast<float>(unscaled_elapsed_time_ms / 1000.0); }
    /// `unscaled_delta_time_ms` in seconds.
    float unscaled_delta_time_sec() const { return static_cast<float>(unscaled_delta_time_ms / 1000.0); }
};

} // namespace ris
