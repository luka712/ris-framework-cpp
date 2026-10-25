#pragma once

// A time source without any platform API, for driving `ris::TimeManager` and `ris::Stopwatch` by hand through the
// `ris::ClockSource` seam. Delays do not sleep: they move the clock forward and are recorded.
#include <cstdint>
#include <vector>

#include <ris/time.hpp>

namespace ris_test {

class FakeClock {
public:
    explicit FakeClock(uint64_t frequency = 1'000'000'000, uint64_t start_ticks = 0)
        : frequency_(frequency), ticks_(start_ticks) {}

    // Not copyable or movable: the source captures `this`.
    FakeClock(const FakeClock&) = delete;
    FakeClock& operator=(const FakeClock&) = delete;

    /// The `ClockSource` to give to a `TimeManager` / `Stopwatch`. It reads and moves this fake, so the fake must
    /// outlive everything that was given the source.
    ris::ClockSource source() {
        ris::ClockSource source;
        source.ticks = [this] { return ticks_; };
        source.ticks_per_second = frequency_;
        source.delay_precise_ns = [this](uint64_t ns) {
            precise_delays.push_back(ns);
            ticks_ += source_view().ns_to_ticks(ns);
        };
        return source;
    }

    uint64_t ticks() const noexcept { return ticks_; }
    uint64_t ticks_per_second() const noexcept { return frequency_; }
    uint64_t ticks_to_ns(uint64_t tick_count) const noexcept { return source_view().ticks_to_ns(tick_count); }
    uint64_t ns_to_ticks(uint64_t ns) const noexcept { return source_view().ns_to_ticks(ns); }
    uint64_t now_ns() const noexcept { return ticks_to_ns(ticks_); }
    double now_seconds() const noexcept { return static_cast<double>(now_ns()) * 1e-9; }

    /// Moves the clock forward by a number of milliseconds.
    void advance_ms(double ms) { ticks_ += ns_to_ticks(ris::time_units::ms_to_ns(ms)); }

    std::vector<uint64_t> precise_delays;

private:
    ris::ClockSource source_view() const {
        ris::ClockSource view;
        view.ticks_per_second = frequency_;
        return view;
    }

    uint64_t frequency_;
    uint64_t ticks_;
};

} // namespace ris_test
