#pragma once

#include <cstdint>
#include <vector>

#include "ris/core.hpp"
#include "ris/input/touch/touch_location.hpp"

namespace ris {

/// The touch locations for one frame, like `TouchCollection` of ris-framework-ts. Slots are allocated once;
/// `clear` and `try_add` reuse them and reads do not allocate. The pointers returned by `get`, `find_by_id` and
/// `try_add` point into those slots: they are valid for the lifetime of the collection but hold the data of the
/// current frame only.
class RIS_API TouchCollection {
public:
    /// Default number of simultaneous touches the collection stores.
    static constexpr int32_t default_max_touch_count = 8;

    /// @param max_touch_count the fixed slot count. Values below 1 store no touches.
    explicit TouchCollection(int32_t max_touch_count = default_max_touch_count);

    /// The fixed number of slots.
    int32_t max_touch_count() const noexcept { return static_cast<int32_t>(locations_.size()); }
    /// The number of touch locations written this frame, including releases.
    int32_t count() const noexcept { return count_; }
    /// The number of touches that were down when they were written: pressed and moved contacts count, released,
    /// cancelled and invalid ones do not.
    int32_t active_count() const noexcept { return active_count_; }

    /// Drops the frame's contacts. The slots stay allocated.
    void clear() noexcept;

    /// Writes the next contact into the next free slot, without a previous sample.
    /// @return the slot that was written, or nullptr when every slot is already used
    TouchLocation* try_add(int32_t id, TouchLocationState state, float x, float y, float pressure = 0);
    /// Writes the next contact with its previous sample (the delta is position minus previous position).
    TouchLocation* try_add(int32_t id, TouchLocationState state, float x, float y, float pressure,
                           TouchLocationState previous_state, float previous_x, float previous_y,
                           float previous_pressure);

    /// The touch at an index in this frame, or nullptr when the index is outside [0, count()).
    const TouchLocation* get(int32_t index) const noexcept;
    /// Finds a touch by id among the contacts written this frame (the first match), or nullptr.
    const TouchLocation* find_by_id(int32_t id) const noexcept;
    /// True when the contact with this id is down. A missing id is not down.
    bool is_down(int32_t id) const noexcept;
    /// True when the contact with this id was released this frame. A missing id was not released.
    bool is_released(int32_t id) const noexcept;
    /// True when the contact with this id was cancelled this frame. A cancel is not a release.
    bool is_cancelled(int32_t id) const noexcept;

private:
    TouchLocation* next_slot() noexcept;
    void commit(TouchLocation& location) noexcept;

    std::vector<TouchLocation> locations_;
    int32_t count_ = 0;
    int32_t active_count_ = 0;
};

} // namespace ris
