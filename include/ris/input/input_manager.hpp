#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "ris/core.hpp"
#include "ris/event.hpp"
#include "ris/input/gamepad/game_pad_state.hpp"
#include "ris/input/keyboard_state.hpp"
#include "ris/input/mouse/mouse_state.hpp"
#include "ris/input/touch/touch_collection.hpp"
#include "ris/input/touch/touch_panel_capabilities.hpp"
#include "ris/math.hpp"

namespace ris {

/// The phases of a touch contact, as reported to `InputManager::report_touch`.
enum class TouchPhase : int32_t {
    Begin = 0,   ///< a finger touched the panel
    Move = 1,    ///< a finger that is down moved
    End = 2,     ///< a finger was lifted
    Cancel = 3   ///< the system cancelled the contact; this is not a lift
};

/// The input manager, like `IInputManager` / `InputManager` of ris-framework-ts: keyboard, mouse, gamepads (SDL
/// gamepad API) and touch (SDL finger events) as per frame state. SDL3 is an integral part of the engine, so this is
/// one concrete class that uses SDL3 directly (there is no abstract input manager and no backend split, only the
/// rendering API is pluggable). No SDL type appears in this header; the SDL headers stay private to the library.
/// `Framework` owns one, get it from `Framework::inputManager()`.
///
/// Frame model (what `Framework` does every frame):
///  1. native events are collected while the window is polled (`Window::poll_event`);
///  2. `update()` publishes everything collected since the previous `update()` as the current state: the
///     keyboard, mouse, gamepad and touch objects are then valid for the whole frame, including the "released"
///     and "pressed" flags, which only live for this one frame;
///  3. the update listeners run and read the state;
///  4. `after_update()` ages per-frame touch state (a pressed contact becomes moved, released ones go away).
///
/// The state objects returned by the getters are owned by the manager, are never invalidated while it lives and
/// always show the latest published frame; keeping a reference is fine. All methods must be called from the
/// thread that runs the frame loop (the thread of the window); nothing is synchronised.
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
/// The `report_*` functions are the platform independent way to feed the state: the SDL event translation is built
/// on them, and they are public so that unit tests and applications with their own event source (a replay, a
/// network input, a different windowing layer) can drive the manager without any window or SDL event. Do not feed
/// the same input both natively and through `report_*` / `handle_event`.
///
/// Lifetime: must be disposed or destroyed before the `ris::Window` (and thereby SDL) goes away; `Framework` does
/// that. Not thread safe: use it on the thread that created the window.
class RIS_API InputManager {
public:
    InputManager();
    ~InputManager();
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    /// The thumbstick dead zone: stick values closer to the centre than this (length of the stick vector, 0 to 1)
    /// are reported as zero. Defaults to 0.2. Applies to the next `update()`.
    float thumbstick_dead_zone() const noexcept { return dead_zone_; }
    /// Sets the thumbstick dead zone. Values are clamped to [0, 1].
    void set_thumbstick_dead_zone(float dead_zone) noexcept;

    /// The keyboard state, to query held, pressed and released keys.
    const KeyboardState& keyboard_state() const noexcept { return keyboard_; }
    /// The mouse state, to query buttons, the cursor position and its movement, and the wheel.
    const MouseState& mouse_state() const noexcept { return mouse_; }
    /// The state of a gamepad.
    /// @param game_pad_index 0 for the first, up to `max_game_pad_count - 1`. Out of range gives a state that is
    ///        not connected.
    const GamePadState& game_pad_state(int32_t game_pad_index = 0) const noexcept;
    /// The touch locations of the current frame.
    const TouchCollection& touch_collection() const noexcept { return touch_collection_; }
    /// The capabilities of the touch panel.
    const TouchPanelCapabilities& touch_capabilities() const noexcept { return touch_capabilities_; }

    /// Starts listening to SDL: registers with the window's event pump, starts the SDL gamepad subsystem, opens the
    /// gamepads that are already connected and reads the touch devices. Safe to call more than once. A missing
    /// gamepad subsystem or device is logged, not an error. `Framework` calls it once after creating the window.
    void initialize();
    /// Publishes the input collected since the last call as the current frame (down / pressed / released sets,
    /// mouse position, delta and wheel, gamepad sticks with the dead zone applied, touch collection) and starts a
    /// new collection period. Call once per frame, before the update listeners.
    void update();
    /// Call once per frame after the update listeners (and before the next `update()`): ages the touch contacts
    /// like `InputManager.afterUpdate` of ris-framework-ts.
    void after_update();
    /// Unregisters from the event pump, closes the gamepads and stops the gamepad subsystem. Safe to call more than
    /// once; called by the destructor. The manager must be disposed (or destroyed) before the window it listens
    /// to is destroyed.
    void dispose();

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

    /// Feeds one platform independent window event: key down / up, mouse move / buttons / wheel and focus lost
    /// are tracked, everything else is ignored. For applications that run their own event loop and for unit tests.
    /// Do not feed events that `initialize()` already receives natively (`Framework` does not call this).
    void handle_event(const Event& event);

    // ---- report_*: platform independent input reports ----------------------------------------------------

    /// A key went down (`down == true`, a repeat of a held key is not a new press) or up. `Key::None` is ignored.
    void report_key(Key key, bool down);
    /// The cursor moved to `position` by `delta`.
    void report_mouse_move(Vec2 position, Vec2 delta);
    /// A mouse button went down or up with the cursor at `position`. `MouseButton::None` is ignored.
    void report_mouse_button(MouseButton button, bool down, Vec2 position);
    /// The wheel moved by `delta` (positive y = away from the user).
    void report_mouse_wheel(Vec2 delta);
    /// The window lost focus: held keys and mouse buttons are released silently (no "released" flag).
    void report_focus_lost();

    /// A gamepad was connected at `index` (0 to `max_game_pad_count - 1`, out of range is ignored) or removed.
    /// Removing a gamepad releases its held buttons and zeroes its sticks.
    void report_game_pad_connected(int32_t index, bool connected);
    /// A gamepad button changed. The gamepad is connected implicitly.
    void report_game_pad_button(int32_t index, GamePadButton button, bool down);
    /// The raw stick and trigger positions of a gamepad (sticks -1 to 1 with y up, triggers 0 to 1). The dead zone
    /// is applied when publishing. The gamepad is connected implicitly.
    void report_game_pad_axes(int32_t index, Vec2 left_stick, Vec2 right_stick, float left_trigger, float right_trigger);

    /// A touch contact event. `id` identifies the contact while it is down; `position` is in window coordinates;
    /// `pressure` is clamped to [0, 1] (0 when unknown). A touch event also marks the touch device connected and
    /// raises the maximum touch count and `has_pressure` of the capabilities when the data shows more.
    void report_touch(TouchPhase phase, int32_t id, Vec2 position, float pressure);
    /// Sets the touch capabilities the platform reports (e.g. at start up). Touch events can only raise them.
    void set_touch_capabilities(bool is_connected, int32_t maximum_touch_count, bool has_pressure);

private:
    struct TouchContact {
        int32_t id = 0;
        bool in_use = false;
        TouchLocationState state = TouchLocationState::Invalid;
        Vec2 position{0, 0};
        float pressure = 0;
        bool has_previous = false;
        bool has_frame_previous = false;
        TouchLocationState previous_state = TouchLocationState::Invalid;
        Vec2 previous_position{0, 0};
        float previous_pressure = 0;
        bool same_frame_released = false;
        void release_slot() noexcept;
    };

    TouchContact* find_contact(int32_t id) noexcept;
    TouchContact* acquire_contact(int32_t id) noexcept;
    static bool is_terminal(const TouchContact& contact) noexcept;
    static void remember_current_as_previous(TouchContact& contact) noexcept;
    void note_touch_device(float pressure);
    void publish_touches();
    void age_touches() noexcept;
    GamePadState* pending_pad(int32_t index) noexcept;

    float dead_zone_ = 0.2f;

    KeyboardState keyboard_, pending_keyboard_;
    MouseState mouse_, pending_mouse_;
    Vec2 pending_delta_{0, 0}, pending_wheel_{0, 0};

    std::array<GamePadState, static_cast<std::size_t>(max_game_pad_count)> pads_, pending_pads_;
    GamePadState disconnected_pad_;

    TouchCollection touch_collection_;
    TouchPanelCapabilities touch_capabilities_;
    std::vector<TouchContact> contacts_;
    bool touch_published_ = false;

    struct Impl;   // the SDL3 state (gamepad slots, finger ids); keeps SDL out of this header
    std::unique_ptr<Impl> impl_;
};

} // namespace ris
