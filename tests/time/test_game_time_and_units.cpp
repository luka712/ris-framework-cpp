#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <limits>

#include <ris/time/time.hpp>

#include "fake_clock.hpp"

using Catch::Approx;

TEST_CASE("GameTime converts milliseconds to seconds like the TypeScript GameTime") {
    ris::GameTime time;
    REQUIRE(time.elapsed_time_ms == 0);
    REQUIRE(time.delta_time_ms == 0);
    REQUIRE(time.elapsed_time_sec() == 0);
    REQUIRE(time.delta_time_sec() == 0);
    REQUIRE(time.frame_number == 0);
    REQUIRE(time.time_scale == 1);
    REQUIRE_FALSE(time.is_paused);
    REQUIRE_FALSE(time.is_running_slowly);

    time.elapsed_time_ms = 1500;
    time.delta_time_ms = 16;
    time.unscaled_elapsed_time_ms = 3000;
    time.unscaled_delta_time_ms = 32;
    REQUIRE(time.elapsed_time_sec() == Approx(1.5f));
    REQUIRE(time.delta_time_sec() == Approx(0.016f));
    REQUIRE(time.unscaled_elapsed_time_sec() == Approx(3.0f));
    REQUIRE(time.unscaled_delta_time_sec() == Approx(0.032f));
}

TEST_CASE("time units") {
    using namespace ris::time_units;
    REQUIRE(ns_to_ms(1'500'000) == Approx(1.5));
    REQUIRE(ns_to_seconds(2'500'000'000) == Approx(2.5));
    REQUIRE(ms_to_ns(1.5) == 1'500'000);
    REQUIRE(seconds_to_ns(0.25) == 250'000'000);
    REQUIRE(ms_to_ns(-3) == 0);
    REQUIRE(ms_to_ns(std::numeric_limits<double>::quiet_NaN()) == 0);
    REQUIRE(seconds_to_ns(-1) == 0);
}

TEST_CASE("AClock converts ticks and nanoseconds at any frequency without overflow") {
    SECTION("1 GHz: ticks are nanoseconds") {
        ris_test::FakeClock clock(1'000'000'000);
        REQUIRE(clock.ticks_to_ns(123'456'789'012'345ull) == 123'456'789'012'345ull);
        REQUIRE(clock.ns_to_ticks(42) == 42);
    }
    SECTION("10 MHz (typical Windows counter) with a counter far beyond 2^64 / 1e9") {
        ris_test::FakeClock clock(10'000'000);
        const uint64_t counter = 9'000'000'000'000'000ull;   // 9e15 ticks = 9e8 seconds
        REQUIRE(clock.ticks_to_ns(counter) == 900'000'000'000'000'000ull);
        REQUIRE(clock.ns_to_ticks(900'000'000'000'000'000ull) == counter);
        REQUIRE(clock.ticks_to_ns(1) == 100);
    }
    SECTION("24 MHz (typical Apple counter): rounds down") {
        ris_test::FakeClock clock(24'000'000);
        REQUIRE(clock.ticks_to_ns(24'000'000) == 1'000'000'000);
        REQUIRE(clock.ticks_to_ns(1) == 41);   // 41.666 ns
        REQUIRE(clock.ns_to_ticks(1'000'000'000) == 24'000'000);
        REQUIRE(clock.ns_to_ticks(1'000'000) == 24'000);
    }
    SECTION("a frequency above 18 GHz still converts") {
        ris_test::FakeClock clock(20'000'000'000ull);
        REQUIRE(clock.ticks_to_ns(20'000'000'000ull) == 1'000'000'000);
        REQUIRE(clock.ticks_to_ns(10'000'000'000ull) == 500'000'000);
        REQUIRE(clock.ns_to_ticks(500'000'000) == 10'000'000'000ull);
    }
    SECTION("helpers") {
        ris_test::FakeClock clock(1'000'000, 2'500'000);   // 1 MHz at 2.5 s
        REQUIRE(clock.now_ns() == 2'500'000'000);
        REQUIRE(clock.now_seconds() == Approx(2.5));
        clock.advance_ms(500);
        REQUIRE(clock.now_seconds() == Approx(3.0));
    }
}

TEST_CASE("DateTime is a comparable value type") {
    ris::DateTime a;
    REQUIRE(a.year == 1970);
    ris::DateTime b = a;
    REQUIRE(a == b);
    b.second = 1;
    REQUIRE_FALSE(a == b);
}
