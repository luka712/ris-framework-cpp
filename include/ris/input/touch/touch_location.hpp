#pragma once

#include <cstdint>
#include <memory>

#include "ris/core.hpp"
#include "ris/input/touch/touch_location_state.hpp"
#include "ris/math.hpp"

namespace ris {

/// A single touch contact, like `TouchLocation` of ris-framework-ts. Position, delta and the previous sample are
/// stored on this object; the locations of a `TouchCollection` are reused every frame, so do not keep pointers to
/// them across frames.
class RIS_API TouchLocation {
public:
    /// @param owns_previous true when this location keeps a previous sample. The previous sample does not keep one
    ///        of its own.
    explicit TouchLocation(bool owns_previous = true);
    TouchLocation(TouchLocation&&) noexcept;
    TouchLocation& operator=(TouchLocation&&) noexcept;
    ~TouchLocation();

    /// The touch id. Stable for the lifetime of the contact; ids are not reused while the contact is down.
    int32_t id = 0;
    /// The pressure, from 0 to 1. 0 when the device does not report pressure.
    float pressure = 0;
    /// The position of the touch, in window coordinates, the same space as `MouseState::position`.
    Vec2 position{0, 0};
    /// The delta from the previous sample. Zero when this frame has no previous sample.
    Vec2 delta{0, 0};

    /// The touch location state.
    TouchLocationState state() const noexcept { return state_; }
    /// Sets the state and updates `is_down()`.
    void set_state(TouchLocationState state) noexcept;

    float x() const noexcept { return position.x; }
    float y() const noexcept { return position.y; }
    float dx() const noexcept { return delta.x; }
    float dy() const noexcept { return delta.y; }

    /// True when the contact is pressed or moved. Updated when the state is written.
    bool is_down() const noexcept { return is_down_; }
    /// True when the contact started this frame.
    bool is_pressed() const noexcept { return state_ == TouchLocationState::Pressed; }
    /// True when the contact moved while held.
    bool is_moved() const noexcept { return state_ == TouchLocationState::Moved; }
    /// True when the contact was released this frame.
    bool is_released() const noexcept { return state_ == TouchLocationState::Released; }
    /// True when the contact was cancelled this frame. A cancel is not a finger lift.
    bool is_cancelled() const noexcept { return state_ == TouchLocationState::Cancelled; }

    /// The previous sample of this contact, or nullptr when this is the first sample. The same object is returned
    /// for every call in the frame.
    const TouchLocation* try_get_previous_location() const noexcept;

    /// Writes this sample. Clears the previous sample and sets the delta to zero.
    void write(int32_t id, TouchLocationState state, float x, float y, float pressure) noexcept;
    /// Writes the previous sample and caches the delta as this position minus that position.
    void write_previous(TouchLocationState state, float x, float y, float pressure) noexcept;

private:
    TouchLocationState state_ = TouchLocationState::Invalid;
    bool is_down_ = false;
    bool has_previous_ = false;
    std::unique_ptr<TouchLocation> previous_;
};

} // namespace ris
