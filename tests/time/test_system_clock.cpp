// ris::system_clock and the system TimeManager against the real SDL timer, wall clock and delays. No window or display needed.
// Tolerances are generous (a loaded CI machine may oversleep) but still catch a unit mistake.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <cstdlib>

#include <ris/ris.hpp>

using Catch::Approx;
using ris::DateTime;
namespace sc = ris::system_clock;

namespace {

// 2023-11-14 22:13:20 UTC, a Tuesday
constexpr int64_t known_unix_seconds = 1'700'000'000;
constexpr int64_t ns_per_second = 1'000'000'000;

} // namespace

TEST_CASE("system_clock counter is monotonic and has a positive frequency") {
    REQUIRE(sc::ticks_per_second() > 0);
    REQUIRE(sc::ticks_per_second() == sc::ticks_per_second());
    REQUIRE(ris::ClockSource::system().ticks_per_second == sc::ticks_per_second());

    uint64_t previous = sc::ticks();
    for (int i = 0; i < 5000; ++i) {
        const uint64_t now = sc::ticks();
        REQUIRE(now >= previous);
        previous = now;
    }
    REQUIRE(sc::now_ns() > 0);

}

TEST_CASE("system_clock counter agrees with the C++ steady clock") {
    const auto cpp_start = std::chrono::steady_clock::now();
    const uint64_t start = sc::now_ns();
    sc::delay_ns(50'000'000);
    const double cpp_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - cpp_start).count();
    const double sdl_ms = static_cast<double>(sc::now_ns() - start) * 1e-6;
    REQUIRE(sdl_ms == Approx(cpp_ms).margin(5.0));
}

TEST_CASE("system_clock delays last at least as long as requested") {

    SECTION("delay_ns") {
        const uint64_t start = sc::now_ns();
        sc::delay_ns(20'000'000);   // 20 ms
        const double ms = static_cast<double>(sc::now_ns() - start) * 1e-6;
        REQUIRE(ms >= 19.5);
        REQUIRE(ms < 120.0);
    }
    SECTION("delay_precise_ns is accurate") {
        for (const uint64_t wanted_ns : {2'000'000ull, 7'500'000ull, 20'000'000ull}) {
            const uint64_t start = sc::now_ns();
            sc::delay_precise_ns(wanted_ns);
            const uint64_t took_ns = sc::now_ns() - start;
            REQUIRE(took_ns >= wanted_ns - wanted_ns / 100);    // never short by more than 1 %
            REQUIRE(took_ns < wanted_ns + 30'000'000ull);       // and not late by more than 30 ms
        }
    }
    SECTION("zero returns at once") {
        const uint64_t start = sc::now_ns();
        sc::delay_ns(0);
        sc::delay_precise_ns(0);
        REQUIRE(sc::now_ns() - start < 20'000'000ull);
    }
}

TEST_CASE("system_clock wall clock") {
    const auto cpp_now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const int64_t sdl_now = sc::unix_time_ns();
    REQUIRE(sdl_now > 0);
    REQUIRE(std::llabs(sdl_now - static_cast<int64_t>(cpp_now)) < 5 * ns_per_second);

    const auto utc = sc::current_date_time(false);
    REQUIRE(utc.has_value());
    REQUIRE(utc->year >= 2025);
    REQUIRE(utc->utc_offset_seconds == 0);
    const auto local = sc::current_date_time(true);
    REQUIRE(local.has_value());
}

TEST_CASE("system_clock converts a Unix time to a calendar date and back") {

    SECTION("the epoch") {
        const auto dt = sc::to_date_time(0, false);
        REQUIRE(dt.has_value());
        REQUIRE(dt->year == 1970);
        REQUIRE(dt->month == 1);
        REQUIRE(dt->day == 1);
        REQUIRE(dt->hour == 0);
        REQUIRE(dt->minute == 0);
        REQUIRE(dt->second == 0);
        REQUIRE(dt->nanosecond == 0);
        REQUIRE(dt->day_of_week == 4);   // Thursday
        REQUIRE(dt->utc_offset_seconds == 0);
        REQUIRE(*dt == DateTime{});      // the default DateTime is the epoch
    }
    SECTION("a known time in UTC") {
        const auto dt = sc::to_date_time(known_unix_seconds * ns_per_second + 123'456'789, false);
        REQUIRE(dt.has_value());
        REQUIRE(dt->year == 2023);
        REQUIRE(dt->month == 11);
        REQUIRE(dt->day == 14);
        REQUIRE(dt->hour == 22);
        REQUIRE(dt->minute == 13);
        REQUIRE(dt->second == 20);
        REQUIRE(dt->nanosecond == 123'456'789);
        REQUIRE(dt->day_of_week == 2);   // Tuesday
        REQUIRE(dt->utc_offset_seconds == 0);

        const auto back = sc::from_date_time(*dt);
        REQUIRE(back.has_value());
        REQUIRE(*back == known_unix_seconds * ns_per_second + 123'456'789);
    }
    SECTION("a negative time (before 1970)") {
        const auto dt = sc::to_date_time(-ns_per_second, false);   // one second before the epoch
        REQUIRE(dt.has_value());
        REQUIRE(dt->year == 1969);
        REQUIRE(dt->month == 12);
        REQUIRE(dt->day == 31);
        REQUIRE(dt->hour == 23);
        REQUIRE(dt->minute == 59);
        REQUIRE(dt->second == 59);
        REQUIRE(dt->day_of_week == 3);   // Wednesday
    }
    SECTION("leap day") {
        DateTime dt;
        dt.year = 2024;
        dt.month = 2;
        dt.day = 29;
        dt.hour = 12;
        const auto unix_ns = sc::from_date_time(dt);
        REQUIRE(unix_ns.has_value());
        REQUIRE(*unix_ns == 1'709'208'000ll * ns_per_second);
        const auto again = sc::to_date_time(*unix_ns, false);
        REQUIRE(again.has_value());
        REQUIRE(again->month == 2);
        REQUIRE(again->day == 29);
        REQUIRE(again->day_of_week == 4);   // Thursday
    }
    SECTION("utc_offset_seconds says which zone the fields are in") {
        DateTime dt;   // 2023-11-15 00:13:20 at UTC+2 is 22:13:20 UTC the day before
        dt.year = 2023;
        dt.month = 11;
        dt.day = 15;
        dt.hour = 0;
        dt.minute = 13;
        dt.second = 20;
        dt.utc_offset_seconds = 2 * 3600;
        const auto unix_ns = sc::from_date_time(dt);
        REQUIRE(unix_ns.has_value());
        REQUIRE(*unix_ns == known_unix_seconds * ns_per_second);
    }
    SECTION("the local time zone round trips") {
        const auto local = sc::to_date_time(known_unix_seconds * ns_per_second, true);
        REQUIRE(local.has_value());
        const auto utc = sc::to_date_time(known_unix_seconds * ns_per_second, false);
        REQUIRE(utc.has_value());
        // the local fields minus the reported offset are the UTC instant
        const auto back = sc::from_date_time(*local);
        REQUIRE(back.has_value());
        REQUIRE(*back == known_unix_seconds * ns_per_second);
        REQUIRE(std::abs(local->utc_offset_seconds) <= 14 * 3600);
    }
    SECTION("out of range fields are rejected") {
        DateTime dt;
        dt.month = 13;
        REQUIRE_FALSE(sc::from_date_time(dt).has_value());
        dt = DateTime{};
        dt.day = 32;
        REQUIRE_FALSE(sc::from_date_time(dt).has_value());
        dt = DateTime{};
        dt.month = 2;
        dt.day = 30;
        REQUIRE_FALSE(sc::from_date_time(dt).has_value());
        dt = DateTime{};
        dt.hour = 24;
        REQUIRE_FALSE(sc::from_date_time(dt).has_value());
        dt = DateTime{};
        dt.nanosecond = 1'000'000'000;
        REQUIRE_FALSE(sc::from_date_time(dt).has_value());
    }
}

TEST_CASE("Stopwatch on the system clock") {
    ris::Stopwatch watch = ris::Stopwatch::start_new();
    sc::delay_ns(30'000'000);
    watch.stop();
    REQUIRE(watch.elapsed_ms() >= 29.5);
    REQUIRE(watch.elapsed_ms() < 150.0);
    const double stopped = watch.elapsed_ms();
    sc::delay_ns(5'000'000);
    REQUIRE(watch.elapsed_ms() == stopped);
}

TEST_CASE("TimeManager on the system clock measures real frames") {
    ris::TimeManager manager_object;
    ris::TimeManager* manager = &manager_object;
    REQUIRE(manager->clock().ticks_per_second == sc::ticks_per_second());

    manager->prepare_start();
    sc::delay_ns(20'000'000);
    manager->frame_start();
    REQUIRE(manager->time().delta_time_ms >= 19.5);
    REQUIRE(manager->time().delta_time_ms < 120.0);
    REQUIRE(manager->time().elapsed_time_ms == Approx(manager->time().delta_time_ms));
    REQUIRE(manager->time().frame_number == 1);

    sc::delay_ns(10'000'000);
    manager->frame_start();
    REQUIRE(manager->time().delta_time_ms >= 9.5);
    REQUIRE(manager->time().elapsed_time_ms >= 29.5);
    REQUIRE(manager->time().frames_per_second > 0);
}

TEST_CASE("TimeManager on the system clock limits the frame rate with a precise delay") {
    ris::TimeManager manager_object;
    ris::TimeManager* manager = &manager_object;
    manager->set_target_frames_per_second(50);   // 20 ms per frame
    manager->prepare_start();

    ris::Stopwatch total = ris::Stopwatch::start_new();
    for (int i = 0; i < 5; ++i) {
        manager->frame_start();
        manager->frame_end();   // the frame body took no time at all
    }
    total.stop();
    REQUIRE(total.elapsed_ms() >= 5 * 20.0 - 25.0);   // the first frame_start has no earlier frame: 4.x periods at least
    REQUIRE(total.elapsed_ms() < 5 * 20.0 + 60.0);
    REQUIRE(manager->time().delta_time_ms == Approx(20.0).margin(8.0));
}

TEST_CASE("ris::time_seconds and sleep_ms use the system clock") {
    const double start = ris::time_seconds();
    ris::sleep_ms(20);
    const double elapsed_ms = (ris::time_seconds() - start) * 1000.0;
    REQUIRE(elapsed_ms >= 19.5);
    REQUIRE(elapsed_ms < 120.0);
    REQUIRE(ris::time_seconds() >= start);
}
