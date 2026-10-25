#pragma once

// A clock without any platform API: shows how another backend plugs in (implement AClock, hand it to
// TimeManagerBase) and lets the tests drive time by hand. Delays do not sleep, they move the clock forward and are
// recorded.
#include <cstdint>
#include <optional>
#include <vector>

#include <ris/time/time.hpp>

namespace ris_test {

class FakeClock final : public ris::AClock {
public:
    explicit FakeClock(uint64_t frequency = 1'000'000'000, uint64_t start_ticks = 0)
        : frequency_(frequency), ticks_(start_ticks) {}

    uint64_t ticks() const noexcept override { return ticks_; }
    uint64_t ticks_per_second() const noexcept override { return frequency_; }
    void delay_ns(uint64_t ns) override { coarse_delays.push_back(ns); ticks_ += ns_to_ticks(ns); }
    void delay_precise_ns(uint64_t ns) override { precise_delays.push_back(ns); ticks_ += ns_to_ticks(ns); }
    int64_t unix_time_ns() const noexcept override { return unix_ns; }
    std::optional<ris::DateTime> to_date_time(int64_t, bool) const override { return std::nullopt; }
    std::optional<int64_t> from_date_time(const ris::DateTime&) const override { return std::nullopt; }

    /// Moves the clock forward by a number of milliseconds.
    void advance_ms(double ms) { ticks_ += ns_to_ticks(ris::time_units::ms_to_ns(ms)); }

    int64_t unix_ns = 0;
    std::vector<uint64_t> coarse_delays;
    std::vector<uint64_t> precise_delays;

private:
    uint64_t frequency_;
    uint64_t ticks_;
};

} // namespace ris_test
