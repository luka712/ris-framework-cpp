#pragma once

#include <cstdint>

#include "ris/core.hpp"
#include "ris/time/clock_source.hpp"
#include "ris/time/frame_rate_counter.hpp"
#include "ris/time/game_time.hpp"

namespace ris {

/// The time manager, like `ITimeManager` / `TimeManager` of ris-framework-ts (`time`, `prepareStart`, `frameStart`)
/// plus time scale, pause, frame limiting, a fixed time step, delta smoothing, an FPS counter and a deterministic
/// mode. It is one concrete class: the engine always runs on SDL3 (windowing, input and time), so only the rendering
/// API is pluggable, not the time source. It measures time with the SDL3 performance counter and waits with
/// `SDL_DelayPrecise` (through `ClockSource::system()`), but names no SDL types in this header. `Framework` owns one
/// and exposes it as `Framework::timeManager()`.
///
/// Tests can drive it without sleeping by passing their own `ClockSource` to the constructor.
///
/// Frame model (what `Framework` does):
///  1. `prepare_start()` once, before the load content listeners;
///  2. every frame: `frame_start()` measures the frame, then the fixed update steps are taken with
///     `consume_fixed_step()` (only when a fixed time step is set), then the update listeners read `time()`;
///  3. after the frame has been presented, `frame_end()` sleeps when a target frame rate is set.
///
/// All methods must be called from the thread that runs the frame loop; nothing is synchronised. Setters throw
/// `Error(Result::InvalidArgument)` for values that make no sense (negative, NaN, infinite) and change nothing.
class RIS_API TimeManager {
public:
    /// Creates a manager on the system clock (`ClockSource::system()`). Ready to use; call `prepare_start()` before
    /// the first `frame_start()` (`Framework` does).
    TimeManager();
    /// Creates a manager on a custom time source, for tests that move time by hand.
    /// @param clock the time source; copied. Whatever its functions capture must outlive the manager.
    /// @throws Error(Result::InvalidArgument) if `clock.ticks` is empty or `clock.ticks_per_second` is 0
    explicit TimeManager(ClockSource clock);

    TimeManager(const TimeManager&) = delete;
    TimeManager& operator=(const TimeManager&) = delete;

    /// The time information of the current frame. The reference stays valid for the life of the manager; its
    /// values change in `frame_start()` and `prepare_start()`.
    const GameTime& time() const noexcept { return time_; }
    /// The clock source this manager reads (`ClockSource::system()` unless one was injected).
    const ClockSource& clock() const noexcept { return clock_; }

    /// Called once before the first frame to start the time: elapsed time, frame count, frame rate and the fixed
    /// step accumulator go back to 0 and the clock is sampled. The settings below (scale, pause, target frame
    /// rate, fixed step, ...) are kept.
    void prepare_start();
    /// Called at the start of every frame: measures the time since the previous `frame_start()` (or
    /// `prepare_start()`) and updates `time()`.
    void frame_start();
    /// Called at the end of every frame, after presenting: when a target frame rate is set, sleeps (precisely,
    /// `ClockSource::delay_precise_ns`) until the frame has lasted `1 / target` seconds. It never sleeps in
    /// deterministic mode or without a target, and does not try to catch up after a long frame.
    void frame_end();

    /// The time scale, 1 by default.
    double time_scale() const noexcept { return time_scale_; }
    /// Sets the time scale: 0.5 is slow motion, 2 is double speed, 0 stops game time (but, unlike `pause()`, is
    /// not reported by `is_paused()`). Scales `delta_time_ms` and `elapsed_time_ms` from the next `frame_start()`;
    /// the unscaled values are not affected. Must be >= 0.
    void set_time_scale(double scale);
    /// True while paused.
    bool is_paused() const noexcept { return paused_; }
    /// Pauses game time: from the next `frame_start()` the scaled delta is 0 and elapsed time stands still; the
    /// unscaled time and the frame count go on. Safe to call when already paused.
    void pause() noexcept { paused_ = true; }
    /// Ends `pause()`. The time spent paused is not counted (no jump). Safe to call when not paused.
    void resume() noexcept { paused_ = false; }

    /// The frame rate limit in frames per second; 0 (the default) means no limit.
    double target_frames_per_second() const noexcept { return target_fps_; }
    /// Sets the frame rate limit used by `frame_end()` and by `GameTime::is_running_slowly`. 0 turns it off. Must
    /// be >= 0. With vsync on the swap already waits for the display; use this when vsync is off or to run below
    /// the refresh rate.
    void set_target_frames_per_second(double frames_per_second);

    /// The length of a fixed update step in milliseconds; 0 (the default) means fixed steps are off.
    double fixed_time_step_ms() const noexcept { return fixed_step_ms_; }
    /// Turns the fixed time step on (> 0, for example `1000.0 / 60.0`) or off (0) and empties its accumulator. Must
    /// be >= 0. With it on, `frame_start()` adds the frame's scaled delta to an accumulator, and the loop takes a
    /// step for every full `fixed_time_step_ms()` in it (`consume_fixed_step()`). Use it for physics and anything
    /// else that needs a constant step.
    void set_fixed_time_step_ms(double step_ms);
    /// The most fixed steps that `frame_start()` allows per frame, 8 by default. When a frame is so long that more
    /// would be needed, the rest of the accumulator is dropped (the game runs slower than real time instead of
    /// freezing in an ever growing catch up) and `GameTime::is_running_slowly` is set.
    uint32_t max_fixed_steps_per_frame() const noexcept { return max_fixed_steps_; }
    /// Sets `max_fixed_steps_per_frame()`. Must be >= 1.
    void set_max_fixed_steps_per_frame(uint32_t max_steps);
    /// Takes one fixed step out of the accumulator, if a full one is available.
    /// @param step receives the step: `elapsed_time_ms` is the fixed (simulation) time at the end of this step,
    ///        `delta_time_ms` is `fixed_time_step_ms()`; all other fields are those of the current frame
    /// @return true if a step was taken (call again until it returns false); always false without a fixed step
    bool consume_fixed_step(GameTime& step) noexcept;
    /// How far the accumulator has progressed into the next fixed step, 0 (inclusive) to 1 (exclusive), valid
    /// after the steps of this frame were consumed. Use it to interpolate rendering between the last two states.
    /// 0 without a fixed step.
    double fixed_step_alpha() const noexcept;

    /// The smoothing factor of `GameTime::smoothed_delta_time_ms`: 0 (the default) turns smoothing off.
    double delta_smoothing() const noexcept { return smoothing_; }
    /// Sets the smoothing factor, 0 <= factor < 1: `smoothed = smoothed * factor + delta * (1 - factor)`, so values
    /// close to 1 smooth more. Must be in that range.
    void set_delta_smoothing(double factor);

    /// The deterministic step in milliseconds; 0 (the default) means the clock is used.
    double deterministic_step_ms() const noexcept { return deterministic_step_ms_; }
    /// Replaces the clock with a constant step, for reproducible runs (screenshots, tests): while > 0, every frame
    /// lasts exactly `step_ms` of unscaled time whatever the clock says, `elapsed_time_ms` is the time at the start
    /// of the frame (the first frame is at 0), and `frame_end()` does not sleep. `Framework` turns it on at 60 Hz
    /// when `FrameworkConfiguration::max_frames` is set. Must be >= 0; 0 goes back to the clock.
    void set_deterministic_step_ms(double step_ms);

private:
    ClockSource clock_;
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
