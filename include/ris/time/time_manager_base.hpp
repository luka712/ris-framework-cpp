#pragma once

#include <cstdint>

#include "ris/core.hpp"
#include "ris/time/a_clock.hpp"
#include "ris/time/a_time_manager.hpp"
#include "ris/time/frame_rate_counter.hpp"

namespace ris {

/// The shared implementation of `ATimeManager` that every backend reuses: delta, elapsed time, frame count, time
/// scale, pause, the fixed step accumulator, frame limiting, delta smoothing, the frame rate and the deterministic
/// mode. It only needs an `AClock`, so it is complete as it is and runs against a fake clock in tests with no
/// platform API at all. A new platform backend implements `AClock` and either passes it to this class or derives
/// from it (like `ris::sdl3::Sdl3TimeManager`), then returns it from `create_default_time_manager()`.
///
/// Not thread safe. The clock must outlive the manager.
class RIS_API TimeManagerBase : public ATimeManager {
public:
    /// @param clock the time source; not owned, must outlive the manager
    explicit TimeManagerBase(AClock& clock);
    ~TimeManagerBase() override = default;

    const GameTime& time() const noexcept override { return time_; }
    const AClock& clock() const noexcept override { return *clock_; }

    void prepare_start() override;
    void frame_start() override;
    void frame_end() override;

    double time_scale() const noexcept override { return time_scale_; }
    void set_time_scale(double scale) override;
    bool is_paused() const noexcept override { return paused_; }
    void pause() noexcept override { paused_ = true; }
    void resume() noexcept override { paused_ = false; }

    double target_frames_per_second() const noexcept override { return target_fps_; }
    void set_target_frames_per_second(double frames_per_second) override;

    double fixed_time_step_ms() const noexcept override { return fixed_step_ms_; }
    void set_fixed_time_step_ms(double step_ms) override;
    uint32_t max_fixed_steps_per_frame() const noexcept override { return max_fixed_steps_; }
    void set_max_fixed_steps_per_frame(uint32_t max_steps) override;
    bool consume_fixed_step(GameTime& step) noexcept override;
    double fixed_step_alpha() const noexcept override;

    double delta_smoothing() const noexcept override { return smoothing_; }
    void set_delta_smoothing(double factor) override;

    double deterministic_step_ms() const noexcept override { return deterministic_step_ms_; }
    void set_deterministic_step_ms(double step_ms) override;

private:
    AClock* clock_;
    GameTime time_;
    FrameRateCounter fps_counter_;

    bool paused_ = false;
    double time_scale_ = 1.0;
    double target_fps_ = 0;
    double fixed_step_ms_ = 0;
    uint32_t max_fixed_steps_ = 8;
    double smoothing_ = 0;
    double deterministic_step_ms_ = 0;

    uint64_t last_ticks_ = 0;
    uint64_t frame_start_ticks_ = 0;
    double last_scaled_delta_ms_ = 0;      // deterministic mode: elapsed lags one frame behind
    double last_unscaled_delta_ms_ = 0;
    double accumulator_ms_ = 0;
    double fixed_elapsed_ms_ = 0;
};

} // namespace ris
