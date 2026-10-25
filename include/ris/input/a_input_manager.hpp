#pragma once

#include <cstdint>

#include "ris/core.hpp"
#include "ris/input/gamepad/game_pad_state.hpp"
#include "ris/input/keyboard_state.hpp"
#include "ris/input/mouse/mouse_state.hpp"
#include "ris/input/touch/touch_collection.hpp"
#include "ris/input/touch/touch_panel_capabilities.hpp"

namespace ris {

/// The input manager, like `IInputManager` of ris-framework-ts. This is an abstract, windowing API neutral class:
/// it names no SDL (or other platform) types, so it can be implemented by any backend (SDL3 in
/// `ris/sdl3/input/`, others later). `Framework` owns one, created by `create_default_input_manager()`; get it
/// from `Framework::input_manager()`.
///
/// Frame model (what `Framework` does every frame):
///  1. the backend collects native events while the window is polled (`Window::poll_event`);
///  2. `update()` publishes everything collected since the previous `update()` as the current state: the
///     keyboard, mouse, gamepad and touch objects are then valid for the whole frame, including the "released"
///     and "pressed" flags, which only live for this one frame;
///  3. the update listeners run and read the state;
///  4. `after_update()` ages per-frame touch state (a pressed contact becomes moved, released ones go away).
///
/// The state objects returned by the getters are owned by the manager, are never invalidated while it lives and
/// always show the latest published frame; keeping a reference is fine. All methods must be called from the
/// thread that runs the frame loop (the thread of the window); nothing is synchronised.
class RIS_API AInputManager {
public:
    virtual ~AInputManager() = default;

    /// The thumbstick dead zone: stick values closer to the centre than this (length of the stick vector, 0 to 1)
    /// are reported as zero. Defaults to 0.2. Applies to the next `update()`.
    virtual float thumbstick_dead_zone() const noexcept = 0;
    /// Sets the thumbstick dead zone. Values are clamped to [0, 1].
    virtual void set_thumbstick_dead_zone(float dead_zone) noexcept = 0;

    /// The keyboard state, to query held, pressed and released keys.
    virtual const KeyboardState& keyboard_state() const noexcept = 0;
    /// The mouse state, to query buttons, the cursor position and its movement, and the wheel.
    virtual const MouseState& mouse_state() const noexcept = 0;
    /// The state of a gamepad.
    /// @param game_pad_index 0 for the first, up to `max_game_pad_count - 1`. Out of range gives a state that is
    ///        not connected.
    virtual const GamePadState& game_pad_state(int32_t game_pad_index = 0) const noexcept = 0;
    /// The touch locations of the current frame.
    virtual const TouchCollection& touch_collection() const noexcept = 0;
    /// The capabilities of the touch panel.
    virtual const TouchPanelCapabilities& touch_capabilities() const noexcept = 0;

    /// Starts listening to the platform: registers with the event source, opens devices that are already
    /// connected. Safe to call more than once. A missing device is not an error. `Framework` calls it once after
    /// creating the window.
    virtual void initialize() = 0;
    /// Publishes the input collected since the last call as the current frame. Call once per frame, before the
    /// update listeners.
    virtual void update() = 0;
    /// Call once per frame after the update listeners (and before the next `update()`): ages the touch contacts.
    virtual void after_update() = 0;
    /// Stops listening and releases devices. Called by the destructor; safe to call more than once. The manager
    /// must be disposed (or destroyed) before the window it listens to is destroyed.
    virtual void dispose() = 0;

    AInputManager() = default;
    AInputManager(const AInputManager&) = delete;
    AInputManager& operator=(const AInputManager&) = delete;
};

} // namespace ris
