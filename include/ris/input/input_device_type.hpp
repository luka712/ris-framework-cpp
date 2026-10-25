#pragma once

#include <cstdint>

namespace ris {

/// The type of an input device, like `InputDeviceType` of ris-framework-ts. The values are bit flags that can be
/// combined with `|`.
enum class InputDeviceType : uint32_t {
    None = 0,            ///< no input
    Keyboard = 1u << 0,
    Mouse = 1u << 1,
    GamePad = 1u << 2,
    Touchpad = 1u << 3,
    Pen = 1u << 4,
    All = 0xFFFFFFFFu    ///< every input device
};

constexpr InputDeviceType operator|(InputDeviceType a, InputDeviceType b) noexcept {
    return static_cast<InputDeviceType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
constexpr InputDeviceType operator&(InputDeviceType a, InputDeviceType b) noexcept {
    return static_cast<InputDeviceType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
/// True when `flags` contains at least one bit of `device`.
constexpr bool has_flag(InputDeviceType flags, InputDeviceType device) noexcept {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(device)) != 0;
}

} // namespace ris
