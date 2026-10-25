#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <utility>
#include <vector>

#include "ris/utilities/listener_id_generator.hpp"

namespace ris {

template <class Signature>
class ListenerMap;

/// Callbacks of one kind of listener, stored in a `std::map` keyed by the `ListenerId` that a
/// `ListenerIdGenerator` gave them.
///
/// A `std::map` is used (and not an `unordered_map`) because it iterates in key order: as ids only grow,
/// listeners are called in the order they were added, which is deterministic. Not thread safe.
///
/// Adding and removing while `invoke` is running is safe:
///  - a listener added during `invoke` is not called by that `invoke` (it is called by the next one),
///  - a listener removed during `invoke` is not called afterwards, even when it was not called yet,
///  - a listener can remove itself; it keeps running until it returns.
///
/// @tparam Args the argument types of the callbacks, which return void.
template <class... Args>
class ListenerMap<void(Args...)> {
public:
    /// The type of the stored callbacks.
    using Callback = std::function<void(Args...)>;

    /// @param generator the generator that gives the ids; it must outlive this map (it is usually a static).
    explicit ListenerMap(ListenerIdGenerator& generator) noexcept : generator_(&generator) {}

    /// Stores a callback under a new id.
    /// @return the id to pass to `remove`. Greater than the id of every earlier listener from the same generator.
    ListenerId add(Callback callback) {
        const ListenerId id = generator_->next();
        callbacks_.emplace_hint(callbacks_.end(), id, std::move(callback));
        return id;
    }

    /// Removes the callback with the given id.
    /// @return true if it existed, false for an unknown or already removed id.
    bool remove(ListenerId id) { return callbacks_.erase(id) > 0; }

    /// Removes all callbacks.
    void clear() noexcept { callbacks_.clear(); }

    /// True when no callback is stored.
    bool empty() const noexcept { return callbacks_.empty(); }

    /// The number of stored callbacks.
    size_t size() const noexcept { return callbacks_.size(); }

    /// Calls every stored callback in id order with the given arguments (see the class description for the rules
    /// about adding and removing meanwhile).
    void invoke(Args... args) {
        if (callbacks_.empty()) return;
        std::vector<ListenerId> ids;
        ids.reserve(callbacks_.size());
        for (const auto& entry : callbacks_) ids.push_back(entry.first);

        for (const ListenerId id : ids) {
            const auto it = callbacks_.find(id);
            if (it == callbacks_.end()) continue;   // removed by an earlier callback
            const Callback callback = it->second;   // a copy: the callback may remove itself
            if (callback) callback(args...);
        }
    }

private:
    ListenerIdGenerator* generator_;
    std::map<ListenerId, Callback> callbacks_;
};

} // namespace ris
