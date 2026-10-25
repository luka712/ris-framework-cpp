#pragma once

#include <atomic>
#include <cstdint>

namespace ris {

/// The id of a registered listener. Returned by every `add...Listener` function and passed to the matching
/// `remove...Listener` function.
using ListenerId = uint32_t;

/// Generates the ids of listeners: 1, 2, 3, ... (0 is never generated, so it can mean "no listener").
///
/// The ids of one generator are unique and strictly increasing, which is what keeps a `ListenerMap` (ordered by id)
/// calling its listeners in registration order. A generator is meant to be a `static` member shared by all the
/// objects that register listeners of the same kind (see `AFramework`), so ids stay unique across instances. It
/// is thread safe and constant initialised, so using it from other static initialisers is safe.
class ListenerIdGenerator {
public:
    constexpr ListenerIdGenerator() noexcept = default;
    ListenerIdGenerator(const ListenerIdGenerator&) = delete;
    ListenerIdGenerator& operator=(const ListenerIdGenerator&) = delete;

    /// Returns the next id: greater than every id returned before, never 0.
    ListenerId next() noexcept { return last_.fetch_add(1, std::memory_order_relaxed) + 1; }

    /// The most recently generated id, or 0 when `next()` has not been called yet.
    ListenerId last() const noexcept { return last_.load(std::memory_order_relaxed); }

private:
    std::atomic<ListenerId> last_{0};
};

} // namespace ris
