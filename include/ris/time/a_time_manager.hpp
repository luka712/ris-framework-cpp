#pragma once

#include <cstdint>

#include "ris/core.hpp"
#include "ris/time/a_clock.hpp"
#include "ris/time/game_time.hpp"

namespace ris {

/// The time manager, like `ITimeManager` of ris-framework-ts (`time`, `prepareStart`, `frameStart`) plus time
/// scale, pause, frame limiting, a fixed time step and a deterministic mode. This is an abstract, platform API
/// neutral class: it names no SDL (or other platform) types. `TimeManagerBase` implements it on top of an
/// `AClock`; `Framework` owns one, created by `create_default_time_manager()`, and exposes it as
/// `Framework::time_manager()`.
///
/// Frame model (what `Framework` does):
///  1. `prepare_start()` once, before the load content listeners;
///  2. every frame: `frame_start()` measures the frame, then the fixed update steps are taken with
///     `consume_fixed_step()` (only when a fixed time step is set), then the update listeners read `time()`;
///  3. after the frame has been presented, `frame_end()` sleeps when a target frame rate is set.
///
/// All methods must be called from the thread that runs the frame loop; nothing is synchronised. Setters throw
/// `Error(Result::InvalidArgument)` for values that make no sense (negative, NaN, infinite) and change nothing.
class RIS_API ATimeManager {
public:
    virtual ~ATimeManager() = default;

    /// The time information of the current frame. The reference stays valid for the life of the manager; its
    /// values change in `frame_start()` and `prepare_start()`.
    virtual const GameTime& time() const noexcept = 0;
    /// The clock this manager reads.
    virtual const AClock& clock() const noexcept = 0;

    /// Called once before the first frame to start the time: elapsed time, frame count, frame rate and the fixed
    /// step accumulator go back to 0 and the clock is sampled. The settings below (scale, pause, target frame
    /// rate, fixed step, ...) are kept.
    virtual void prepare_start() = 0;
    /// Called at the start of every frame: measures the time since the previous `frame_start()` (or
    /// `prepare_start()`) and updates `time()`.
    virtual void frame_start() = 0;
    /// Called at the end of every frame, after presenting: when a target frame rate is set, sleeps (precisely,
    /// `AClock::delay_precise_ns`) until the frame has lasted `1 / target` seconds. It never sleeps in
    /// deterministic mode or without a target, and does not try to catch up after a long frame.
    virtual void frame_end() = 0;

    /// The time scale, 1 by default.
    virtual double time_scale() const noexcept = 0;
    /// Sets the time scale: 0.5 is slow motion, 2 is double speed, 0 stops game time (but, unlike `pause()`, is
    /// not reported by `is_paused()`). Scales `delta_time_ms` and `elapsed_time_ms` from the next `frame_start()`;
    /// the unscaled values are not affected. Must be >= 0.
    virtual void set_time_scale(double scale) = 0;
    /// True while paused.
    virtual bool is_paused() const noexcept = 0;
    /// Pauses game time: from the next `frame_start()` the scaled delta is 0 and elapsed time stands still; the
    /// unscaled time and the frame count go on. Safe to call when already paused.
    virtual void pause() noexcept = 0;
    /// Ends `pause()`. The time spent paused is not counted (no jump). Safe to call when not paused.
    virtual void resume() noexcept = 0;

    /// The frame rate limit in frames per second; 0 (the default) means no limit.
    virtual double target_frames_per_second() const noexcept = 0;
    /// Sets the frame rate limit used by `frame_end()` and by `GameTime::is_running_slowly`. 0 turns it off. Must
    /// be >= 0. With vsync on the swap already waits for the display; use this when vsync is off or to run below
    /// the refresh rate.
    virtual void set_target_frames_per_second(double frames_per_second) = 0;

    /// The length of a fixed update step in milliseconds; 0 (the default) means fixed steps are off.
    virtual double fixed_time_step_ms() const noexcept = 0;
    /// Turns the fixed time step on (> 0, for example `1000.0 / 60.0`) or off (0) and empties its accumulator. Must
    /// be >= 0. With it on, `frame_start()` adds the frame's scaled delta to an accumulator, and the loop takes a
    /// step for every full `fixed_time_step_ms()` in it (`consume_fixed_step()`). Use it for physics and anything
    /// else that needs a constant step.
    virtual void set_fixed_time_step_ms(double step_ms) = 0;
    /// The most fixed steps that `frame_start()` allows per frame, 8 by default. When a frame is so long that more
    /// would be needed, the rest of the accumulator is dropped (the game runs slower than real time instead of
    /// freezing in an ever growing catch up) and `GameTime::is_running_slowly` is set.
    virtual uint32_t max_fixed_steps_per_frame() const noexcept = 0;
    /// Sets `max_fixed_steps_per_frame()`. Must be >= 1.
    virtual void set_max_fixed_steps_per_frame(uint32_t max_steps) = 0;
    /// Takes one fixed step out of the accumulator, if a full one is available.
    /// @param step receives the step: `elapsed_time_ms` is the fixed (simulation) time at the end of this step,
    ///        `delta_time_ms` is `fixed_time_step_ms()`; all other fields are those of the current frame
    /// @return true if a step was taken (call again until it returns false); always false without a fixed step
    virtual bool consume_fixed_step(GameTime& step) noexcept = 0;
    /// How far the accumulator has progressed into the next fixed step, 0 (inclusive) to 1 (exclusive), valid
    /// after the steps of this frame were consumed. Use it to interpolate rendering between the last two states.
    /// 0 without a fixed step.
    virtual double fixed_step_alpha() const noexcept = 0;

    /// The smoothing factor of `GameTime::smoothed_delta_time_ms`: 0 (the default) turns smoothing off.
    virtual double delta_smoothing() const noexcept = 0;
    /// Sets the smoothing factor, 0 <= factor < 1: `smoothed = smoothed * factor + delta * (1 - factor)`, so values
    /// close to 1 smooth more. Must be in that range.
    virtual void set_delta_smoothing(double factor) = 0;

    /// The deterministic step in milliseconds; 0 (the default) means the clock is used.
    virtual double deterministic_step_ms() const noexcept = 0;
    /// Replaces the clock with a constant step, for reproducible runs (screenshots, tests): while > 0, every frame
    /// lasts exactly `step_ms` of unscaled time whatever the clock says, `elapsed_time_ms` is the time at the start
    /// of the frame (the first frame is at 0), and `frame_end()` does not sleep. `Framework` turns it on at 60 Hz
    /// when `FrameworkConfiguration::max_frames` is set. Must be >= 0; 0 goes back to the clock.
    virtual void set_deterministic_step_ms(double step_ms) = 0;

    ATimeManager() = default;
    ATimeManager(const ATimeManager&) = delete;
    ATimeManager& operator=(const ATimeManager&) = delete;
};

} // namespace ris
