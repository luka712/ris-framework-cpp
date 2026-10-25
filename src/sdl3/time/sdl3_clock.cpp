#include "ris/sdl3/time/sdl3_clock.hpp"

#include <SDL3/SDL.h>

namespace ris::sdl3 {

Sdl3Clock& Sdl3Clock::shared() noexcept {
    static Sdl3Clock clock;
    return clock;
}

uint64_t Sdl3Clock::ticks() const noexcept { return SDL_GetPerformanceCounter(); }

uint64_t Sdl3Clock::ticks_per_second() const noexcept {
    const uint64_t frequency = SDL_GetPerformanceFrequency();
    return frequency > 0 ? frequency : 1;   // SDL never reports 0; stay safe against dividing by it
}

void Sdl3Clock::delay_ns(uint64_t ns) {
    if (ns > 0) {
        SDL_DelayNS(ns);
    }
}

void Sdl3Clock::delay_precise_ns(uint64_t ns) {
    if (ns > 0) {
        SDL_DelayPrecise(ns);
    }
}

int64_t Sdl3Clock::unix_time_ns() const noexcept {
    SDL_Time now = 0;
    return SDL_GetCurrentTime(&now) ? static_cast<int64_t>(now) : 0;
}

std::optional<DateTime> Sdl3Clock::to_date_time(int64_t unix_time_ns, bool local) const {
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

std::optional<int64_t> Sdl3Clock::from_date_time(const DateTime& date_time) const {
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

} // namespace ris::sdl3
