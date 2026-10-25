#include "ris/time/time_manager_base.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "ris/time/time_units.hpp"

namespace ris {

namespace {

// A frame counts as running slowly when it lasted longer than this many target frame times.
constexpr double slow_frame_factor = 1.5;
// Tolerance (milliseconds) so that an accumulator that is a rounding error short of a step still takes the step.
constexpr double step_epsilon_ms = 1e-9;

void require_finite_non_negative(double value, const char* what) {
    if (!(value >= 0.0) || std::isinf(value)) {
        throw Error(Result::InvalidArgument, std::string(what) + " must be a finite number >= 0");
    }
}

} // namespace

TimeManagerBase::TimeManagerBase(AClock& clock) : clock_(&clock) {
    last_ticks_ = frame_start_ticks_ = clock_->ticks();
}

void TimeManagerBase::prepare_start() {
    time_ = {};
    time_.time_scale = time_scale_;
    time_.is_paused = paused_;
    fps_counter_.reset();
    accumulator_ms_ = 0;
    fixed_elapsed_ms_ = 0;
    last_scaled_delta_ms_ = 0;
    last_unscaled_delta_ms_ = 0;
    last_ticks_ = frame_start_ticks_ = clock_->ticks();
}

void TimeManagerBase::frame_start() {
    const uint64_t now = clock_->ticks();
    frame_start_ticks_ = now;

    double unscaled_delta_ms;
    if (deterministic_step_ms_ > 0.0) {
        unscaled_delta_ms = deterministic_step_ms_;
    } else {
        unscaled_delta_ms = time_units::ns_to_ms(clock_->ticks_to_ns(now > last_ticks_ ? now - last_ticks_ : 0));
    }
    last_ticks_ = now;

    ++time_.frame_number;
    const double scaled_delta_ms = paused_ ? 0.0 : unscaled_delta_ms * time_scale_;

    // Deterministic mode keeps the elapsed time one frame behind: it is the time at the start of the frame.
    const bool lagging = deterministic_step_ms_ > 0.0;
    time_.elapsed_time_ms += lagging ? last_scaled_delta_ms_ : scaled_delta_ms;
    time_.unscaled_elapsed_time_ms += lagging ? last_unscaled_delta_ms_ : unscaled_delta_ms;
    last_scaled_delta_ms_ = scaled_delta_ms;
    last_unscaled_delta_ms_ = unscaled_delta_ms;

    time_.delta_time_ms = scaled_delta_ms;
    time_.unscaled_delta_time_ms = unscaled_delta_ms;
    time_.time_scale = time_scale_;
    time_.is_paused = paused_;
    time_.smoothed_delta_time_ms = (time_.frame_number == 1 || smoothing_ <= 0.0)
                                       ? unscaled_delta_ms
                                       : time_.smoothed_delta_time_ms * smoothing_ + unscaled_delta_ms * (1.0 - smoothing_);

    fps_counter_.add_frame(unscaled_delta_ms);
    time_.frames_per_second = fps_counter_.frames_per_second();

    bool slow = target_fps_ > 0.0 && unscaled_delta_ms > slow_frame_factor * 1000.0 / target_fps_;
    if (fixed_step_ms_ > 0.0) {
        accumulator_ms_ += scaled_delta_ms;
        const double cap_ms = fixed_step_ms_ * static_cast<double>(max_fixed_steps_);
        if (accumulator_ms_ > cap_ms) {
            accumulator_ms_ = cap_ms;
            slow = true;
        }
    }
    time_.is_running_slowly = slow && time_.frame_number > 1;
}

void TimeManagerBase::frame_end() {
    if (deterministic_step_ms_ > 0.0 || target_fps_ <= 0.0) {
        return;
    }
    const uint64_t period_ticks = clock_->ns_to_ticks(time_units::seconds_to_ns(1.0 / target_fps_));
    const uint64_t due = frame_start_ticks_ + period_ticks;
    const uint64_t now = clock_->ticks();
    if (now < due) {
        clock_->delay_precise_ns(clock_->ticks_to_ns(due - now));
    }
}

void TimeManagerBase::set_time_scale(double scale) {
    require_finite_non_negative(scale, "time scale");
    time_scale_ = scale;
}

void TimeManagerBase::set_target_frames_per_second(double frames_per_second) {
    require_finite_non_negative(frames_per_second, "target frames per second");
    target_fps_ = frames_per_second;
}

void TimeManagerBase::set_fixed_time_step_ms(double step_ms) {
    require_finite_non_negative(step_ms, "fixed time step");
    fixed_step_ms_ = step_ms;
    accumulator_ms_ = 0;
}

void TimeManagerBase::set_max_fixed_steps_per_frame(uint32_t max_steps) {
    if (max_steps < 1) {
        throw Error(Result::InvalidArgument, "max fixed steps per frame must be >= 1");
    }
    max_fixed_steps_ = max_steps;
}

bool TimeManagerBase::consume_fixed_step(GameTime& step) noexcept {
    if (fixed_step_ms_ <= 0.0 || accumulator_ms_ + step_epsilon_ms < fixed_step_ms_) {
        return false;
    }
    accumulator_ms_ = std::max(0.0, accumulator_ms_ - fixed_step_ms_);
    fixed_elapsed_ms_ += fixed_step_ms_;
    step = time_;
    step.elapsed_time_ms = fixed_elapsed_ms_;
    step.delta_time_ms = fixed_step_ms_;
    return true;
}

double TimeManagerBase::fixed_step_alpha() const noexcept {
    if (fixed_step_ms_ <= 0.0) {
        return 0.0;
    }
    return std::max(0.0, accumulator_ms_ / fixed_step_ms_);
}

void TimeManagerBase::set_delta_smoothing(double factor) {
    if (!(factor >= 0.0 && factor < 1.0)) {
        throw Error(Result::InvalidArgument, "delta smoothing must be >= 0 and < 1");
    }
    smoothing_ = factor;
}

void TimeManagerBase::set_deterministic_step_ms(double step_ms) {
    require_finite_non_negative(step_ms, "deterministic step");
    deterministic_step_ms_ = step_ms;
}

} // namespace ris
