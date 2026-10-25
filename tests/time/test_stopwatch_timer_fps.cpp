#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <limits>

#include <ris/time.hpp>

#include "fake_clock.hpp"

using Catch::Approx;

TEST_CASE("Stopwatch accumulates between start and stop") {
    ris_test::FakeClock clock(1'000'000, 123'456);   // 1 MHz, odd start
    ris::Stopwatch watch(clock.source());
    REQUIRE_FALSE(watch.is_running());
    REQUIRE(watch.elapsed_ns() == 0);

    clock.advance_ms(10);
    REQUIRE(watch.elapsed_ms() == 0);   // not started

    watch.start();
    REQUIRE(watch.is_running());
    clock.advance_ms(250);
    REQUIRE(watch.elapsed_ms() == Approx(250.0));
    REQUIRE(watch.elapsed_ns() == 250'000'000);
    REQUIRE(watch.elapsed_seconds() == Approx(0.25));

    watch.start();   // already running: no restart
    clock.advance_ms(50);
    watch.stop();
    REQUIRE_FALSE(watch.is_running());
    REQUIRE(watch.elapsed_ms() == Approx(300.0));

    clock.advance_ms(1000);   // stopped: not counted
    REQUIRE(watch.elapsed_ms() == Approx(300.0));
    watch.stop();             // stopping twice is harmless
    REQUIRE(watch.elapsed_ms() == Approx(300.0));

    watch.start();            // resumes and accumulates
    clock.advance_ms(100);
    REQUIRE(watch.elapsed_ms() == Approx(400.0));

    watch.restart();
    REQUIRE(watch.is_running());
    REQUIRE(watch.elapsed_ms() == 0);
    clock.advance_ms(5);
    REQUIRE(watch.elapsed_ms() == Approx(5.0));

    watch.reset();
    REQUIRE_FALSE(watch.is_running());
    REQUIRE(watch.elapsed_ns() == 0);

    auto running = ris::Stopwatch::start_new(clock.source());
    REQUIRE(running.is_running());
    clock.advance_ms(1);
    REQUIRE(running.elapsed_ms() == Approx(1.0));
}

TEST_CASE("Timer one shot") {
    REQUIRE_THROWS_AS(ris::Timer(0), ris::Error);
    REQUIRE_THROWS_AS(ris::Timer(-5), ris::Error);
    REQUIRE_THROWS_AS(ris::Timer(std::numeric_limits<double>::quiet_NaN()), ris::Error);
    REQUIRE_THROWS_AS(ris::Timer(std::numeric_limits<double>::infinity()), ris::Error);

    ris::Timer timer(100.0);
    REQUIRE(timer.is_running());
    REQUIRE_FALSE(timer.is_repeating());
    REQUIRE(timer.update(40) == 0);
    REQUIRE(timer.elapsed_ms() == Approx(40));
    REQUIRE(timer.remaining_ms() == Approx(60));
    REQUIRE(timer.progress() == Approx(0.4));
    REQUIRE(timer.update(-10) == 0);   // negative counts as 0
    REQUIRE(timer.update(std::numeric_limits<double>::quiet_NaN()) == 0);
    REQUIRE(timer.elapsed_ms() == Approx(40));

    REQUIRE(timer.update(500) == 1);   // fires once however far it is overshot
    REQUIRE(timer.is_finished());
    REQUIRE_FALSE(timer.is_running());
    REQUIRE(timer.progress() == 1.0);
    REQUIRE(timer.update(100) == 0);
    timer.start();                     // a finished timer does not restart by itself
    REQUIRE_FALSE(timer.is_running());

    timer.reset();
    REQUIRE_FALSE(timer.is_finished());
    timer.start();
    REQUIRE(timer.update(100) == 1);
}

TEST_CASE("Timer repeating and stopped") {
    ris::Timer timer(100.0, true, false);
    REQUIRE_FALSE(timer.is_running());
    REQUIRE(timer.update(1000) == 0);   // not started

    timer.start();
    REQUIRE(timer.update(60) == 0);
    REQUIRE(timer.update(60) == 1);
    REQUIRE(timer.elapsed_ms() == Approx(20));
    REQUIRE(timer.update(350) == 3);    // a long frame fires several times
    REQUIRE(timer.elapsed_ms() == Approx(70));
    REQUIRE_FALSE(timer.is_finished());

    timer.stop();
    REQUIRE(timer.update(1000) == 0);
    REQUIRE(timer.elapsed_ms() == Approx(70));
    timer.start();
    REQUIRE(timer.update(30) == 1);
    timer.reset();
    REQUIRE(timer.elapsed_ms() == 0);
}

TEST_CASE("FrameRateCounter") {
    ris::FrameRateCounter counter;
    REQUIRE(counter.interval_ms() == 500.0);
    REQUIRE(counter.frames_per_second() == 0);

    counter.add_frame(0);   // no time yet
    REQUIRE(counter.frames_per_second() == 0);

    counter.reset();
    counter.add_frame(100);   // provisional average before the first window completes
    REQUIRE(counter.frames_per_second() == Approx(10.0));

    for (int i = 0; i < 4; ++i) counter.add_frame(100);   // 500 ms: window completes with 5 frames
    REQUIRE(counter.frames_per_second() == Approx(10.0));

    for (int i = 0; i < 25; ++i) counter.add_frame(20.0);   // 500 ms
    REQUIRE(counter.frames_per_second() == Approx(50.0));

    counter.add_frame(1.0);   // the next window is still running: the published value stays
    REQUIRE(counter.frames_per_second() == Approx(50.0));

    counter.reset();
    REQUIRE(counter.frames_per_second() == 0);

    ris::FrameRateCounter bad(-1);
    REQUIRE(bad.interval_ms() == 500.0);
    ris::FrameRateCounter custom(1000.0);
    REQUIRE(custom.interval_ms() == 1000.0);
    custom.add_frame(std::numeric_limits<double>::quiet_NaN());   // counts as a frame of 0 ms
    REQUIRE(custom.frames_per_second() == 0);
}
