#pragma once

#include <cstdint>
#include <optional>

#include "ris/core.hpp"
#include "ris/time/a_clock.hpp"

namespace ris::sdl3 {

/// The SDL3 implementation of `ris::AClock`. It is stateless, so any number of instances can exist (the
/// framework shares `ris::default_clock()`), and every function is thread safe. No SDL type appears in this
/// header; SDL does not need to be initialised.
///
/// Mapping:
///  - `ticks()` / `ticks_per_second()`: `SDL_GetPerformanceCounter()` / `SDL_GetPerformanceFrequency()` (the
///    monotonic high resolution clock of the platform; 1 GHz on Linux, QueryPerformanceCounter on Windows);
///  - `delay_ns()`: `SDL_DelayNS()`; `delay_precise_ns()`: `SDL_DelayPrecise()` (sleeps most of the time, then
///    spins);
///  - `unix_time_ns()`: `SDL_GetCurrentTime()`; `to_date_time()` / `from_date_time()`: `SDL_TimeToDateTime()` /
///    `SDL_DateTimeToTime()`. `SDL_DateTimeToTime()` honours `utc_offset_seconds`; the local time zone is the
///    one SDL / the C library report.
class RIS_API Sdl3Clock final : public AClock {
public:
    Sdl3Clock() = default;

    /// The shared instance that `ris::default_clock()` returns and `Sdl3TimeManager` reads. Lives until the program
    /// ends.
    static Sdl3Clock& shared() noexcept;

    uint64_t ticks() const noexcept override;
    uint64_t ticks_per_second() const noexcept override;
    void delay_ns(uint64_t ns) override;
    void delay_precise_ns(uint64_t ns) override;
    int64_t unix_time_ns() const noexcept override;
    std::optional<DateTime> to_date_time(int64_t unix_time_ns, bool local) const override;
    std::optional<int64_t> from_date_time(const DateTime& date_time) const override;
};

} // namespace ris::sdl3
