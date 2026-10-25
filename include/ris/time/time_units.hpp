#pragma once

#include <cstdint>

namespace ris::time_units {

/// Nanoseconds in one microsecond.
inline constexpr uint64_t ns_per_us = 1'000;
/// Nanoseconds in one millisecond.
inline constexpr uint64_t ns_per_ms = 1'000'000;
/// Nanoseconds in one second.
inline constexpr uint64_t ns_per_second = 1'000'000'000;

/// Converts nanoseconds to (fractional) milliseconds.
constexpr double ns_to_ms(uint64_t ns) noexcept { return static_cast<double>(ns) * 1e-6; }
/// Converts nanoseconds to (fractional) seconds.
constexpr double ns_to_seconds(uint64_t ns) noexcept { return static_cast<double>(ns) * 1e-9; }
/// Converts (fractional) milliseconds to nanoseconds. Negative and NaN values give 0.
constexpr uint64_t ms_to_ns(double ms) noexcept {
    return ms > 0.0 ? static_cast<uint64_t>(ms * 1e6) : 0u;
}
/// Converts (fractional) seconds to nanoseconds. Negative and NaN values give 0.
constexpr uint64_t seconds_to_ns(double seconds) noexcept {
    return seconds > 0.0 ? static_cast<uint64_t>(seconds * 1e9) : 0u;
}

} // namespace ris::time_units
