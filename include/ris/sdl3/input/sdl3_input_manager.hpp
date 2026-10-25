#pragma once

#include <memory>

#include "ris/core.hpp"
#include "ris/input/input_manager_base.hpp"
#include "ris/math.hpp"

namespace ris::sdl3 {

/// The SDL3 implementation of the input manager (`ris::AInputManager`): keyboard, mouse, gamepads (SDL gamepad
/// API) and touch (SDL finger events). It derives from the shared `InputManagerBase`, so it only translates SDL
/// events into the neutral `report_*` calls; the per-frame state, dead zone and touch ageing are shared with every
/// other backend. No SDL type appears in this header.
///
/// How it gets events: `initialize()` registers the manager with the `ris::Window` event pump, so every SDL event
/// that `Window::poll_event` (and therefore the framework loop) pulls from SDL is also shown to the manager. An
/// application that pumps SDL itself can call `handle_native_event` instead.
///
/// Mapping:
///  - keys: SDL scancodes are the USB HID usages of `ris::Key`; mouse: SDL button 1/2/3/4/5 = left / middle /
///    right / X1 / X2; positions are SDL window coordinates; the wheel is `SDL_MouseWheelEvent::x/y` as is.
///  - touch: each SDL finger gets a small integer contact id (never reused while the finger is down); the
///    normalised SDL position is scaled to window coordinates (the size of the window the event reports, or
///    `set_touch_surface_size` when SDL reports no window), the same space as the mouse. SDL's touch to mouse
///    emulation stays on, so a finger also moves the mouse like in the browser.
///  - gamepads: the first four connected gamepads get indices 0 to 3 in connection order and keep them until they
///    are removed. SDL's south / east / west / north buttons are A / B / X / Y (Xbox layout), guide is BigButton,
///    other buttons (touchpad, paddles, ...) are ignored. SDL stick y (down positive) is flipped so that y is
///    positive up, triggers are 0 to 1. With no gamepad connected, or if SDL cannot start the gamepad subsystem,
///    `game_pad_state()` reports not connected.
///
/// Lifetime: must be disposed or destroyed before the `ris::Window` (and thereby SDL) goes away; `Framework` does
/// that. Not thread safe: use it on the thread that created the window.
class RIS_API Sdl3InputManager final : public InputManagerBase {
public:
    Sdl3InputManager();
    ~Sdl3InputManager() override;

    /// Registers with the window's event pump, starts the SDL gamepad subsystem, opens the gamepads that are
    /// already connected and reads the touch devices. Safe to call more than once. A missing gamepad subsystem or
    /// device is logged, not an error.
    void initialize() override;
    /// Unregisters from the event pump, closes the gamepads and stops the gamepad subsystem. Safe to call more
    /// than once; called by the destructor.
    void dispose() override;

    /// Handles one native SDL event.
    /// @param sdl_event pointer to an `SDL_Event` (of the SDL3 the framework is built with); not retained
    /// @return true when the event is input that the manager consumed (keyboard, mouse, touch, gamepad, focus lost)
    ///         and false for every other event, which is not touched. The event is not removed from SDL's queue.
    /// Called by the window's event pump after `initialize()`; call it yourself only if you pump SDL outside of
    /// `Window::poll_event`, and then do not call `initialize()`'s registration twice for the same events.
    bool handle_native_event(const void* sdl_event);

    /// The size touch positions are scaled to when SDL reports a finger event without a window. Defaults to
    /// (1, 1), i.e. positions stay normalised. Events that name a window use that window's size instead.
    void set_touch_surface_size(Vec2 size) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ris::sdl3
