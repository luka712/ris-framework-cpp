#include "ris/input/touch/touch_collection.hpp"

namespace ris {

TouchCollection::TouchCollection(int32_t max_touch_count) {
    const size_t capacity = max_touch_count > 0 ? static_cast<size_t>(max_touch_count) : 0;
    locations_.reserve(capacity);
    for (size_t i = 0; i < capacity; ++i) {
        locations_.emplace_back();
    }
}

void TouchCollection::clear() noexcept {
    count_ = 0;
    active_count_ = 0;
}

TouchLocation* TouchCollection::next_slot() noexcept {
    if (count_ >= static_cast<int32_t>(locations_.size())) {
        return nullptr;
    }
    return &locations_[static_cast<size_t>(count_)];
}

void TouchCollection::commit(TouchLocation& location) noexcept {
    ++count_;
    if (location.is_down()) {
        ++active_count_;
    }
}

TouchLocation* TouchCollection::try_add(int32_t id, TouchLocationState state, float x, float y, float pressure) {
    TouchLocation* location = next_slot();
    if (!location) {
        return nullptr;
    }
    location->write(id, state, x, y, pressure);
    commit(*location);
    return location;
}

TouchLocation* TouchCollection::try_add(int32_t id, TouchLocationState state, float x, float y, float pressure,
                                        TouchLocationState previous_state, float previous_x, float previous_y,
                                        float previous_pressure) {
    TouchLocation* location = next_slot();
    if (!location) {
        return nullptr;
    }
    location->write(id, state, x, y, pressure);
    location->write_previous(previous_state, previous_x, previous_y, previous_pressure);
    commit(*location);
    return location;
}

const TouchLocation* TouchCollection::get(int32_t index) const noexcept {
    if (index < 0 || index >= count_) {
        return nullptr;
    }
    return &locations_[static_cast<size_t>(index)];
}

const TouchLocation* TouchCollection::find_by_id(int32_t id) const noexcept {
    for (int32_t i = 0; i < count_; ++i) {
        const TouchLocation& location = locations_[static_cast<size_t>(i)];
        if (location.id == id) {
            return &location;
        }
    }
    return nullptr;
}

bool TouchCollection::is_down(int32_t id) const noexcept {
    const TouchLocation* location = find_by_id(id);
    return location && location->is_down();
}

bool TouchCollection::is_released(int32_t id) const noexcept {
    const TouchLocation* location = find_by_id(id);
    return location && location->is_released();
}

bool TouchCollection::is_cancelled(int32_t id) const noexcept {
    const TouchLocation* location = find_by_id(id);
    return location && location->is_cancelled();
}

} // namespace ris
