#pragma once

#include <cstdint>

namespace ris {

/// Physical keyboard keys, like `Key` of ris-framework-ts (which only has a handful of them; this is a superset).
///
/// The values are the USB HID usage ids of the keyboard page (the same numbers as SDL scancodes and as
/// `Event::scancode`), so they are independent of the keyboard layout and every backend can map its native key
/// codes to them. `Key::None` (0) means "no key" and is never reported as pressed.
///
/// Names of ris-framework-ts: `LEFTARROW` = `Left`, `RIGHTARROW` = `Right`, `UPARROW` = `Up`, `DOWNARROW` = `Down`,
/// `CONTROLLEFT` / `CONTROLRIGHT` = `LeftCtrl` / `RightCtrl`, `SHIFTLEFT` / `SHIFTRIGHT` = `LeftShift` /
/// `RightShift`, `NUMPAD0` ... `NUMPAD9`, `NUMPADSUBTRACT`, `NUMPADADD`, `MINUS`, `EQUAL`, `DELETE`.
enum class Key : int32_t {
    None = 0,
    A = 4, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num1 = 30, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Num0,
    Enter = 40, Escape = 41, Backspace = 42, Tab = 43, Space = 44,
    Minus = 45, Equal = 46, LeftBracket = 47, RightBracket = 48, Backslash = 49,
    Semicolon = 51, Apostrophe = 52, Grave = 53, Comma = 54, Period = 55, Slash = 56, CapsLock = 57,
    F1 = 58, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    PrintScreen = 70, ScrollLock = 71, Pause = 72,
    Insert = 73, Home = 74, PageUp = 75, Delete = 76, End = 77, PageDown = 78,
    Right = 79, Left = 80, Down = 81, Up = 82,
    NumLock = 83, NumpadDivide = 84, NumpadMultiply = 85, NumpadSubtract = 86, NumpadAdd = 87, NumpadEnter = 88,
    Numpad1 = 89, Numpad2, Numpad3, Numpad4, Numpad5, Numpad6, Numpad7, Numpad8, Numpad9, Numpad0 = 98,
    NumpadDecimal = 99,
    LeftCtrl = 224, LeftShift = 225, LeftAlt = 226, LeftGui = 227,
    RightCtrl = 228, RightShift = 229, RightAlt = 230, RightGui = 231
};

/// Number of key slots a keyboard state tracks: every scancode in [0, key_count) can be queried.
inline constexpr int32_t key_count = 512;

/// Converts a scancode (USB HID usage, e.g. `Event::scancode`) to a Key. Returns `Key::None` when it is outside
/// [1, key_count). Scancodes that have no named enumerator are still returned (cast), so they can be tracked.
constexpr Key key_from_scancode(int32_t scancode) noexcept {
    return (scancode > 0 && scancode < key_count) ? static_cast<Key>(scancode) : Key::None;
}

} // namespace ris
