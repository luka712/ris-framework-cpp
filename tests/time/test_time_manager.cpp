// TimeManager driven by a fake ClockSource: fully deterministic, no sleeping and no display.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>

#include <ris/time.hpp>

#include "fake_clock.hpp"

using Catch::Approx;
using ris::GameTime;
using ris::TimeManager;

TEST_CASE("TimeManager mirrors the TypeScript TimeManager") {
    // Same sequence as tests/time.test.ts of ris-framework-ts, with a clock in place of Date.now.
    ris_test::FakeClock clock(1'000'000'000, 5'000'000'000ull);
    TimeManager manager(clock.source());

    manager.prepare_start();
    REQUIRE(manager.time().elapsed_time_ms == 0);
    REQUIRE(manager.time().delta_time_ms == 0);

    clock.advance_ms(600);
    manager.frame_start();
    REQUIRE(manager.time().delta_time_ms == Approx(600));
    REQUIRE(manager.time().elapsed_time_ms == Approx(600));
    REQUIRE(manager.time().delta_time_sec() == Approx(0.6f));
    REQUIRE(manager.time().elapsed_time_sec() == Approx(0.6f));

    clock.advance_ms(150);
    manager.frame_start();
    REQUIRE(manager.time().delta_time_ms == Approx(150));
    REQUIRE(manager.time().elapsed_time_ms == Approx(750));

    clock.advance_ms(3000);
    manager.prepare_start();   // restarts elapsed time (the C++ version also clears the delta)
    REQUIRE(manager.time().elapsed_time_ms == 0);
    REQUIRE(manager.time().frame_number == 0);

    clock.advance_ms(100);
    manager.frame_start();
    REQUIRE(manager.time().delta_time_ms == Approx(100));
    REQUIRE(manager.time().elapsed_time_ms == Approx(100));
    REQUIRE(manager.time().frame_number == 1);
    REQUIRE(manager.clock().ticks() == clock.ticks());
    REQUIRE(manager.clock().ticks_per_second == 1'000'000'000);
}

TEST_CASE("TimeManager works at any clock frequency") {
    ris_test::FakeClock clock(24'000'000, 7);   // 24 MHz
    TimeManager manager(clock.source());
    manager.prepare_start();
    clock.advance_ms(16);
    manager.frame_start();
    REQUIRE(manager.time().delta_time_ms == Approx(16.0).epsilon(0.001));
}

TEST_CASE("TimeManager defaults") {
    ris_test::FakeClock clock;
    TimeManager manager(clock.source());
    REQUIRE(manager.time_scale() == 1.0);
    REQUIRE_FALSE(manager.is_paused());
    REQUIRE(manager.target_frames_per_second() == 0);
    REQUIRE(manager.fixed_time_step_ms() == 0);
    REQUIRE(manager.max_fixed_steps_per_frame() == 8);
    REQUIRE(manager.delta_smoothing() == 0);
    REQUIRE(manager.deterministic_step_ms() == 0);
    GameTime step;
    REQUIRE_FALSE(manager.consume_fixed_step(step));   // no fixed step, no steps
    REQUIRE(manager.fixed_step_alpha() == 0);
}

TEST_CASE("time scale and pause change the scaled time only") {
    ris_test::FakeClock clock;
    TimeManager manager(clock.source());
    manager.prepare_start();

    clock.advance_ms(100);
    manager.frame_start();
    REQUIRE(manager.time().delta_time_ms == Approx(100));

    manager.set_time_scale(0.5);
    clock.advance_ms(100);
    manager.frame_start();
    REQUIRE(manager.time().time_scale == 0.5);
    REQUIRE(manager.time().delta_time_ms == Approx(50));
    REQUIRE(manager.time().unscaled_delta_time_ms == Approx(100));
    REQUIRE(manager.time().elapsed_time_ms == Approx(150));
    REQUIRE(manager.time().unscaled_elapsed_time_ms == Approx(200));

    manager.set_time_scale(2.0);
    clock.advance_ms(100);
    manager.frame_start();
    REQUIRE(manager.time().delta_time_ms == Approx(200));
    REQUIRE(manager.time().elapsed_time_ms == Approx(350));

    manager.set_time_scale(1.0);
    manager.pause();
    manager.pause();   // harmless twice
    REQUIRE(manager.is_paused());
    clock.advance_ms(1000);
    manager.frame_start();
    REQUIRE(manager.time().is_paused);
    REQUIRE(manager.time().delta_time_ms == 0);
    REQUIRE(manager.time().elapsed_time_ms == Approx(350));
    REQUIRE(manager.time().unscaled_delta_time_ms == Approx(1000));
    REQUIRE(manager.time().unscaled_elapsed_time_ms == Approx(1300));
    REQUIRE(manager.time().frame_number == 4);   // frames keep counting

    manager.resume();
    REQUIRE_FALSE(manager.is_paused());
    clock.advance_ms(16);
    manager.frame_start();
    REQUIRE_FALSE(manager.time().is_paused);
    REQUIRE(manager.time().delta_time_ms == Approx(16));   // the time spent paused is not counted
    REQUIRE(manager.time().elapsed_time_ms == Approx(366));

    manager.set_time_scale(0);   // scale 0 stops game time without being "paused"
    clock.advance_ms(16);
    manager.frame_start();
    REQUIRE(manager.time().delta_time_ms == 0);
    REQUIRE_FALSE(manager.time().is_paused);

    // prepare_start keeps the settings
    manager.set_time_scale(3.0);
    manager.pause();
    manager.prepare_start();
    REQUIRE(manager.time_scale() == 3.0);
    REQUIRE(manager.is_paused());
    REQUIRE(manager.time().time_scale == 3.0);
    REQUIRE(manager.time().is_paused);
}

TEST_CASE("setters reject invalid values and change nothing") {
    ris_test::FakeClock clock;
    TimeManager manager(clock.source());
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();

    REQUIRE_THROWS_AS(manager.set_time_scale(-1), ris::Error);
    REQUIRE_THROWS_AS(manager.set_time_scale(nan), ris::Error);
    REQUIRE_THROWS_AS(manager.set_time_scale(inf), ris::Error);
    REQUIRE(manager.time_scale() == 1.0);

    REQUIRE_THROWS_AS(manager.set_target_frames_per_second(-30), ris::Error);
    REQUIRE_THROWS_AS(manager.set_target_frames_per_second(nan), ris::Error);
    REQUIRE(manager.target_frames_per_second() == 0);

    REQUIRE_THROWS_AS(manager.set_fixed_time_step_ms(-1), ris::Error);
    REQUIRE_THROWS_AS(manager.set_fixed_time_step_ms(inf), ris::Error);
    REQUIRE(manager.fixed_time_step_ms() == 0);

    REQUIRE_THROWS_AS(manager.set_max_fixed_steps_per_frame(0), ris::Error);
    REQUIRE(manager.max_fixed_steps_per_frame() == 8);

    REQUIRE_THROWS_AS(manager.set_delta_smoothing(-0.1), ris::Error);
    REQUIRE_THROWS_AS(manager.set_delta_smoothing(1.0), ris::Error);
    REQUIRE_THROWS_AS(manager.set_delta_smoothing(nan), ris::Error);
    REQUIRE(manager.delta_smoothing() == 0);

    REQUIRE_THROWS_AS(manager.set_deterministic_step_ms(-1), ris::Error);
    REQUIRE(manager.deterministic_step_ms() == 0);

    try {
        manager.set_time_scale(-1);
        FAIL("expected an exception");
    } catch (const ris::Error& error) {
        REQUIRE(error.code() == ris::Result::InvalidArgument);
    }
}

TEST_CASE("fixed time step accumulates and is consumed in whole steps") {
    ris_test::FakeClock clock;
    TimeManager manager(clock.source());
    manager.set_fixed_time_step_ms(10.0);
    manager.prepare_start();

    GameTime step;
    clock.advance_ms(25);
    manager.frame_start();
    int steps = 0;
    while (manager.consume_fixed_step(step)) {
        ++steps;
        REQUIRE(step.delta_time_ms == 10.0);
        REQUIRE(step.elapsed_time_ms == Approx(10.0 * steps));   // fixed simulation time
        REQUIRE(step.frame_number == 1);                          // other fields are the frame's
        REQUIRE(step.unscaled_delta_time_ms == Approx(25));
    }
    REQUIRE(steps == 2);
    REQUIRE(manager.fixed_step_alpha() == Approx(0.5));
    REQUIRE_FALSE(manager.consume_fixed_step(step));

    clock.advance_ms(5);   // 5 left over + 5 = exactly one more step
    manager.frame_start();
    REQUIRE(manager.consume_fixed_step(step));
    REQUIRE(step.elapsed_time_ms == Approx(30.0));
    REQUIRE_FALSE(manager.consume_fixed_step(step));
    REQUIRE(manager.fixed_step_alpha() == Approx(0.0).margin(1e-9));
    REQUIRE_FALSE(manager.time().is_running_slowly);

    // the scaled time feeds the accumulator: half speed gives half the steps, a pause gives none
    manager.set_time_scale(0.5);
    clock.advance_ms(40);
    manager.frame_start();
    steps = 0;
    while (manager.consume_fixed_step(step)) ++steps;
    REQUIRE(steps == 2);
    manager.pause();
    clock.advance_ms(1000);
    manager.frame_start();
    REQUIRE_FALSE(manager.consume_fixed_step(step));

    // turning it off empties the accumulator
    manager.resume();
    manager.set_time_scale(1.0);
    clock.advance_ms(9);
    manager.frame_start();
    manager.set_fixed_time_step_ms(0);
    REQUIRE_FALSE(manager.consume_fixed_step(step));
    REQUIRE(manager.fixed_step_alpha() == 0);
}

TEST_CASE("fixed time step drops time instead of spiralling") {
    ris_test::FakeClock clock;
    TimeManager manager(clock.source());
    manager.set_fixed_time_step_ms(10.0);
    manager.set_max_fixed_steps_per_frame(3);
    manager.prepare_start();

    clock.advance_ms(5000);   // the first frame is never reported as slow (it contains loading)
    manager.frame_start();
    GameTime step;
    int steps = 0;
    while (manager.consume_fixed_step(step)) ++steps;
    REQUIRE(steps == 3);
    REQUIRE_FALSE(manager.time().is_running_slowly);

    clock.advance_ms(1000);   // second frame: 100 steps needed, 3 allowed, the rest is dropped
    manager.frame_start();
    REQUIRE(manager.time().is_running_slowly);
    steps = 0;
    while (manager.consume_fixed_step(step)) ++steps;
    REQUIRE(steps == 3);
    REQUIRE(manager.fixed_step_alpha() == Approx(0.0).margin(1e-9));

    clock.advance_ms(10);     // back to normal
    manager.frame_start();
    REQUIRE_FALSE(manager.time().is_running_slowly);
    steps = 0;
    while (manager.consume_fixed_step(step)) ++steps;
    REQUIRE(steps == 1);
}

TEST_CASE("deterministic mode ignores the clock and keeps elapsed time at the start of the frame") {
    ris_test::FakeClock clock;
    TimeManager manager(clock.source());
    const double step_ms = 1000.0 / 60.0;
    manager.set_deterministic_step_ms(step_ms);
    manager.prepare_start();

    for (uint64_t frame = 1; frame <= 5; ++frame) {
        clock.advance_ms(123.0 * static_cast<double>(frame));   // whatever the clock says
        manager.frame_start();
        REQUIRE(manager.time().frame_number == frame);
        REQUIRE(manager.time().delta_time_ms == step_ms);
        REQUIRE(manager.time().unscaled_delta_time_ms == step_ms);
        REQUIRE(manager.time().elapsed_time_ms == Approx(static_cast<double>(frame - 1) * step_ms).epsilon(1e-12).margin(1e-12));
        REQUIRE(manager.time().unscaled_elapsed_time_ms == Approx(static_cast<double>(frame - 1) * step_ms).epsilon(1e-12).margin(1e-12));
    }
    REQUIRE(manager.time().frames_per_second == Approx(60.0).epsilon(1e-6));

    manager.frame_end();   // never sleeps in deterministic mode, even with a target
    manager.set_target_frames_per_second(30);
    manager.frame_end();
    REQUIRE(clock.precise_delays.empty());

    // time scale and pause still apply
    manager.set_time_scale(0.5);
    manager.frame_start();
    REQUIRE(manager.time().delta_time_ms == Approx(step_ms / 2));
    manager.set_deterministic_step_ms(0);   // back to the clock
    clock.advance_ms(7);
    manager.frame_start();
    REQUIRE(manager.time().unscaled_delta_time_ms == Approx(7.0));
}

TEST_CASE("frame_end limits the frame rate") {
    ris_test::FakeClock clock;
    TimeManager manager(clock.source());

    manager.prepare_start();
    clock.advance_ms(3);
    manager.frame_start();
    manager.frame_end();   // no target: no sleep
    REQUIRE(clock.precise_delays.empty());

    manager.set_target_frames_per_second(100);   // 10 ms per frame
    clock.advance_ms(10);
    manager.frame_start();
    clock.advance_ms(4);   // the frame took 4 ms
    manager.frame_end();
    REQUIRE(clock.precise_delays.size() == 1);
    REQUIRE(clock.precise_delays[0] == Approx(6'000'000.0).epsilon(1e-6));

    clock.advance_ms(0);
    manager.frame_start();   // the next frame starts exactly at the end of the sleep: delta is 10 ms
    REQUIRE(manager.time().delta_time_ms == Approx(10.0));
    REQUIRE_FALSE(manager.time().is_running_slowly);
    clock.advance_ms(25);    // a frame that took longer than the budget: no sleep, no catch up
    manager.frame_end();
    REQUIRE(clock.precise_delays.size() == 1);
    manager.frame_start();
    REQUIRE(manager.time().is_running_slowly);   // 25 ms > 1.5 * 10 ms
    manager.frame_end();
    REQUIRE(clock.precise_delays.size() == 2);
    REQUIRE(clock.precise_delays[1] == Approx(10'000'000.0).epsilon(1e-6));

    manager.set_target_frames_per_second(0);   // off again
    manager.frame_end();
    REQUIRE(clock.precise_delays.size() == 2);
}

TEST_CASE("running slowly needs a target frame rate, and never flags the first frame") {
    ris_test::FakeClock clock;
    TimeManager manager(clock.source());
    manager.prepare_start();
    clock.advance_ms(500);
    manager.frame_start();
    REQUIRE_FALSE(manager.time().is_running_slowly);
    clock.advance_ms(500);
    manager.frame_start();
    REQUIRE_FALSE(manager.time().is_running_slowly);   // no target

    manager.set_target_frames_per_second(60);
    clock.advance_ms(500);
    manager.frame_start();
    REQUIRE(manager.time().is_running_slowly);
    clock.advance_ms(16);
    manager.frame_start();
    REQUIRE_FALSE(manager.time().is_running_slowly);
}

TEST_CASE("delta smoothing is an exponential moving average of the unscaled delta") {
    ris_test::FakeClock clock;
    TimeManager manager(clock.source());
    manager.prepare_start();

    clock.advance_ms(10);
    manager.frame_start();
    REQUIRE(manager.time().smoothed_delta_time_ms == Approx(10));   // off: equals the raw delta
    clock.advance_ms(30);
    manager.frame_start();
    REQUIRE(manager.time().smoothed_delta_time_ms == Approx(30));

    manager.set_delta_smoothing(0.5);
    manager.prepare_start();
    clock.advance_ms(10);
    manager.frame_start();
    REQUIRE(manager.time().smoothed_delta_time_ms == Approx(10));   // the first frame seeds it
    clock.advance_ms(30);
    manager.frame_start();
    REQUIRE(manager.time().smoothed_delta_time_ms == Approx(20));   // 10 * 0.5 + 30 * 0.5
    REQUIRE(manager.time().delta_time_ms == Approx(30));            // the delta itself is not smoothed
    clock.advance_ms(30);
    manager.frame_start();
    REQUIRE(manager.time().smoothed_delta_time_ms == Approx(25));
    manager.set_time_scale(0.1);   // smoothing is of the unscaled delta
    clock.advance_ms(30);
    manager.frame_start();
    REQUIRE(manager.time().smoothed_delta_time_ms == Approx(27.5));
}

TEST_CASE("frames per second") {
    ris_test::FakeClock clock;
    TimeManager manager(clock.source());
    manager.prepare_start();
    REQUIRE(manager.time().frames_per_second == 0);
    for (int i = 0; i < 40; ++i) {
        clock.advance_ms(20);
        manager.frame_start();
    }
    REQUIRE(manager.time().frames_per_second == Approx(50.0));
    for (int i = 0; i < 25; ++i) {
        clock.advance_ms(40);   // the frame rate follows unscaled time, not the time scale
        manager.set_time_scale(0.0);
        manager.frame_start();
    }
    REQUIRE(manager.time().frames_per_second == Approx(25.0));
    manager.prepare_start();
    REQUIRE(manager.time().frames_per_second == 0);
}

TEST_CASE("TimeManager rejects an unusable clock source") {
    ris::ClockSource no_ticks;
    REQUIRE_THROWS_AS(TimeManager(no_ticks), ris::Error);

    ris_test::FakeClock clock;
    ris::ClockSource zero_frequency = clock.source();
    zero_frequency.ticks_per_second = 0;
    REQUIRE_THROWS_AS(TimeManager(zero_frequency), ris::Error);
    REQUIRE_THROWS_AS(ris::Stopwatch(zero_frequency), ris::Error);

    ris::ClockSource no_delay = clock.source();
    no_delay.delay_precise_ns = nullptr;   // allowed: frame_end just does not wait
    TimeManager manager(no_delay);
    manager.set_target_frames_per_second(60);
    manager.prepare_start();
    manager.frame_start();
    REQUIRE_NOTHROW(manager.frame_end());
}
