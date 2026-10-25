#pragma once

#include <cstdint>

namespace ris {

/// A calendar date and time (Gregorian calendar), the result of `AClock::to_date_time()` and the input of
/// `AClock::from_date_time()`. A plain value type; it names no platform type.
struct DateTime {
    int32_t year = 1970;          ///< the year, for example 2026
    int32_t month = 1;            ///< month of the year, 1 (January) to 12
    int32_t day = 1;              ///< day of the month, 1 to 31
    int32_t hour = 0;             ///< hour of the day, 0 to 23
    int32_t minute = 0;           ///< minute, 0 to 59
    int32_t second = 0;           ///< second, 0 to 60 (60 only for a leap second)
    int32_t nanosecond = 0;       ///< nanosecond of the second, 0 to 999'999'999
    int32_t day_of_week = 4;      ///< 0 (Sunday) to 6 (Saturday); ignored by `from_date_time`
    int32_t utc_offset_seconds = 0; ///< seconds the time is ahead of UTC (east positive); 0 for UTC

    friend constexpr bool operator==(const DateTime&, const DateTime&) = default;
};

} // namespace ris
