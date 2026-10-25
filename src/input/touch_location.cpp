#include "ris/input/touch/touch_location.hpp"

namespace ris {

TouchLocation::TouchLocation(bool owns_previous) {
    if (owns_previous) {
        previous_ = std::make_unique<TouchLocation>(false);
    }
}

TouchLocation::TouchLocation(TouchLocation&&) noexcept = default;
TouchLocation& TouchLocation::operator=(TouchLocation&&) noexcept = default;
TouchLocation::~TouchLocation() = default;

void TouchLocation::set_state(TouchLocationState state) noexcept {
    state_ = state;
    is_down_ = state == TouchLocationState::Pressed || state == TouchLocationState::Moved;
}

const TouchLocation* TouchLocation::try_get_previous_location() const noexcept {
    return has_previous_ ? previous_.get() : nullptr;
}

void TouchLocation::write(int32_t new_id, TouchLocationState new_state, float x, float y, float new_pressure) noexcept {
    id = new_id;
    set_state(new_state);
    position = {x, y};
    pressure = new_pressure;
    has_previous_ = false;
    delta = {0, 0};
}

void TouchLocation::write_previous(TouchLocationState previous_state, float x, float y, float previous_pressure) noexcept {
    if (!previous_) {
        return;
    }
    previous_->write(id, previous_state, x, y, previous_pressure);
    has_previous_ = true;
    delta = {position.x - x, position.y - y};
}

} // namespace ris
