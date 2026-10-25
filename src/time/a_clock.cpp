#include "ris/time/a_clock.hpp"

#include "ris/time/time_units.hpp"

namespace ris {

namespace {

// Frequencies up to this keep `remainder * 1e9` inside 64 bits (remainder < frequency).
constexpr uint64_t max_exact_frequency = UINT64_MAX / time_units::ns_per_second;

} // namespace

uint64_t AClock::ticks_to_ns(uint64_t tick_count) const noexcept {
    const uint64_t frequency = ticks_per_second();
    if (frequency == 0) {
        return 0;
    }
    const uint64_t whole = tick_count / frequency;
    const uint64_t rest = tick_count % frequency;
    const uint64_t rest_ns = frequency <= max_exact_frequency
                                 ? rest * time_units::ns_per_second / frequency
                                 : static_cast<uint64_t>(static_cast<long double>(rest) * 1e9L / static_cast<long double>(frequency));
    return whole * time_units::ns_per_second + rest_ns;
}

uint64_t AClock::ns_to_ticks(uint64_t ns) const noexcept {
    const uint64_t frequency = ticks_per_second();
    const uint64_t whole = ns / time_units::ns_per_second;
    const uint64_t rest = ns % time_units::ns_per_second;
    const uint64_t rest_ticks = frequency <= max_exact_frequency
                                    ? rest * frequency / time_units::ns_per_second
                                    : static_cast<uint64_t>(static_cast<long double>(rest) * static_cast<long double>(frequency) / 1e9L);
    return whole * frequency + rest_ticks;
}

} // namespace ris
