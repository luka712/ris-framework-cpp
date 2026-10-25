#include "sdl3/sdl3_event_tap.hpp"

#include <list>
#include <utility>

namespace ris::sdl3::detail {

namespace {
struct Entry {
    const void* owner;
    EventTap tap;
};
struct State {
    std::list<Entry> list;   // stable addresses: a tap may add taps while it runs
    int dispatching = 0;   // > 0 while taps run; removals are then deferred
};
State& state() {
    static State s;
    return s;
}
void compact(State& s) {
    s.list.remove_if([](const Entry& e) { return e.owner == nullptr; });
}
} // namespace

void add_event_tap(const void* owner, EventTap tap) {
    State& s = state();
    for (Entry& entry : s.list) {
        if (entry.owner == owner) {
            entry.tap = std::move(tap);
            return;
        }
    }
    s.list.push_back({owner, std::move(tap)});   // taps added while dispatching see events from the next one on
}

void remove_event_tap(const void* owner) {
    State& s = state();
    if (s.dispatching > 0) {
        for (Entry& entry : s.list) {
            if (entry.owner == owner) {
                entry.owner = nullptr;   // the tap object may be running right now, keep it alive until the end
            }
        }
        return;
    }
    s.list.remove_if([&](const Entry& e) { return e.owner == owner; });
}

void dispatch_event_tap(const SDL_Event& event) {
    State& s = state();
    if (s.list.empty()) {
        return;
    }
    ++s.dispatching;
    const size_t count = s.list.size();   // taps added during dispatch run for the next event
    auto it = s.list.begin();
    for (size_t i = 0; i < count; ++i, ++it) {
        if (it->owner != nullptr) {
            it->tap(event);
        }
    }
    if (--s.dispatching == 0) {
        compact(s);
    }
}

} // namespace ris::sdl3::detail
