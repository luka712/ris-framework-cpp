#pragma once

#include <cstdint>

#include "ris/core.hpp"

namespace ris {

/// Capabilities of the touch panel, like `TouchPanelCapabilities` of ris-framework-ts. The input manager owns one
/// instance (`AInputManager::touch_capabilities()`) and updates it in place when a device appears, so a reference
/// to it stays current.
class RIS_API TouchPanelCapabilities {
public:
    TouchPanelCapabilities() = default;
    /// @param is_connected true when a touch device is connected
    /// @param maximum_touch_count the maximum number of simultaneous touches, 0 when none are supported
    /// @param has_pressure true when contacts report pressure
    TouchPanelCapabilities(bool is_connected, int32_t maximum_touch_count, bool has_pressure) noexcept
        : is_connected_(is_connected), maximum_touch_count_(maximum_touch_count), has_pressure_(has_pressure) {}

    bool is_connected() const noexcept { return is_connected_; }
    int32_t maximum_touch_count() const noexcept { return maximum_touch_count_; }
    bool has_pressure() const noexcept { return has_pressure_; }

    /// Writes new capabilities into this instance.
    void set(bool is_connected, int32_t maximum_touch_count, bool has_pressure) noexcept {
        is_connected_ = is_connected;
        maximum_touch_count_ = maximum_touch_count;
        has_pressure_ = has_pressure;
    }

private:
    bool is_connected_ = false;
    int32_t maximum_touch_count_ = 0;
    bool has_pressure_ = false;
};

} // namespace ris
