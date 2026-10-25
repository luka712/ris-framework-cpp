#pragma once

#include <cstdint>

namespace ris {

/// Kinds of window / input events delivered by `Window::poll_event`. The values match the C API's
/// `ris_event_type`.
enum class EventType : uint32_t {
    None = 0,
    Quit = 1,
    WindowResized = 2,      ///< width, height = new drawable size in pixels
    WindowFocusGained = 3,
    WindowFocusLost = 4,
    KeyDown = 5,            ///< scancode, keycode, mods, repeat
    KeyUp = 6,
    TextInput = 7,          ///< text
    MouseMove = 8,          ///< x, y, dx, dy
    MouseButtonDown = 9,    ///< button, clicks, x, y
    MouseButtonUp = 10,
    MouseWheel = 11         ///< dx, dy
};

namespace key_mod {
inline constexpr uint32_t shift = 1u << 0;
inline constexpr uint32_t ctrl = 1u << 1;
inline constexpr uint32_t alt = 1u << 2;
inline constexpr uint32_t gui = 1u << 3;
} // namespace key_mod

/// Same layout as the C API's `ris_event`. Mouse buttons: 1 = left, 2 = middle, 3 = right, 4 and 5 = extra buttons.
/// The event is a platform independent description; it carries no SDL types. Touch and gamepad input are not part
/// of it (the layout is fixed by the C ABI): they are delivered straight to the input manager of the backend.
struct Event {
    EventType type = EventType::None;
    uint32_t repeat = 0;
    uint64_t timestamp_ns = 0;
    int32_t scancode = 0;   ///< USB HID usage, the values of `ris::Key`
    int32_t keycode = 0;    ///< layout dependent key (SDL_Keycode)
    uint32_t mods = 0;      ///< key_mod bit mask
    int32_t button = 0;
    float x = 0, y = 0, dx = 0, dy = 0;
    int32_t width = 0, height = 0;
    int32_t clicks = 0;
    int32_t reserved = 0;
    char text[32] = {};     ///< UTF-8, EventType::TextInput only
};

} // namespace ris
