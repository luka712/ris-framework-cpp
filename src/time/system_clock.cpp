#include "ris/time/system_clock.hpp"

#include <SDL3/SDL.h>

#include "ris/time/clock_source.hpp"

namespace ris::system_clock {

uint64_t ticks() noexcept { return SDL_GetPerformanceCounter(); }

uint64_t ticks_per_second() noexcept {
    const uint64_t frequency = SDL_GetPerformanceFrequency();
    return frequency > 0 ? frequency : 1;   // SDL never reports 0; stay safe against dividing by it
}

uint64_t now_ns() noexcept {
    ClockSource source;
    source.ticks_per_second = ticks_per_second();
    return source.ticks_to_ns(ticks());
}

double now_seconds() noexcept { return static_cast<double>(now_ns()) * 1e-9; }

void delay_ns(uint64_t ns) noexcept {
    if (ns > 0) {
        SDL_DelayNS(ns);
    }
}

void delay_precise_ns(uint64_t ns) noexcept {
    if (ns > 0) {
        SDL_DelayPrecise(ns);
    }
}

int64_t unix_time_ns() noexcept {
    SDL_Time now = 0;
    return SDL_GetCurrentTime(&now) ? static_cast<int64_t>(now) : 0;
}

std::optional<DateTime> to_date_time(int64_t unix_time_ns, bool local) noexcept {
    SDL_DateTime dt;
    SDL_zero(dt);
    if (!SDL_TimeToDateTime(static_cast<SDL_Time>(unix_time_ns), &dt, local)) {
        return std::nullopt;
    }
    DateTime result;
    result.year = dt.year;
    result.month = dt.month;
    result.day = dt.day;
    result.hour = dt.hour;
    result.minute = dt.minute;
    result.second = dt.second;
    result.nanosecond = dt.nanosecond;
    result.day_of_week = dt.day_of_week;
    result.utc_offset_seconds = dt.utc_offset;
    return result;
}

std::optional<int64_t> from_date_time(const DateTime& date_time) noexcept {
    SDL_DateTime dt;
    SDL_zero(dt);
    dt.year = date_time.year;
    dt.month = date_time.month;
    dt.day = date_time.day;
    dt.hour = date_time.hour;
    dt.minute = date_time.minute;
    dt.second = date_time.second;
    dt.nanosecond = date_time.nanosecond;
    dt.day_of_week = date_time.day_of_week;
    dt.utc_offset = date_time.utc_offset_seconds;
    SDL_Time result = 0;
    if (!SDL_DateTimeToTime(&dt, &result)) {
        return std::nullopt;
    }
    return static_cast<int64_t>(result);
}

std::optional<DateTime> current_date_time(bool local) noexcept { return to_date_time(unix_time_ns(), local); }

} // namespace ris::system_clock
