#pragma once

#include <cstddef>
#include <array>
#include <cstdint>
#include <vector>

#include "ris/core.hpp"
#include "ris/event.hpp"
#include "ris/input/a_input_manager.hpp"

namespace ris {

/// The phases of a touch contact, as reported by a backend to `InputManagerBase::report_touch`.
enum class TouchPhase : int32_t {
    Begin = 0,   ///< a finger touched the panel
    Move = 1,    ///< a finger that is down moved
    End = 2,     ///< a finger was lifted
    Cancel = 3   ///< the system cancelled the contact; this is not a lift
};

/// The shared, windowing API neutral half of every input manager: it implements everything of `AInputManager`
/// except the platform part (`initialize` and `dispose`), and turns the platform independent "reports" of a backend
/// into per-frame state:
///  - pending state is collected between frames from the `report_*` calls;
///  - `update()` publishes it (down / pressed / released sets, mouse position, delta and wheel, gamepad sticks
///    with the dead zone applied, touch collection) and starts a new collection period;
///  - `after_update()` ages the touch contacts like `InputManager.afterUpdate` of ris-framework-ts.
///
/// A backend (see `ris::sdl3::Sdl3InputManager`) derives from this class, translates its native events to
/// `report_*` calls and implements `initialize` / `dispose`. Nothing here depends on a platform API, so the whole
/// state logic is tested with a fake backend. Not thread safe: call everything from the frame loop's thread.
class RIS_API InputManagerBase : public AInputManager {
public:
    InputManagerBase();
    ~InputManagerBase() override;

    float thumbstick_dead_zone() const noexcept override { return dead_zone_; }
    void set_thumbstick_dead_zone(float dead_zone) noexcept override;
    const KeyboardState& keyboard_state() const noexcept override { return keyboard_; }
    const MouseState& mouse_state() const noexcept override { return mouse_; }
    const GamePadState& game_pad_state(int32_t game_pad_index = 0) const noexcept override;
    const TouchCollection& touch_collection() const noexcept override { return touch_collection_; }
    const TouchPanelCapabilities& touch_capabilities() const noexcept override { return touch_capabilities_; }
    void update() override;
    void after_update() override;

    /// Feeds one platform independent window event: key down / up, mouse move / buttons / wheel and focus lost
    /// are tracked, everything else is ignored. For applications that run their own event loop and for backends
    /// that already translate to `Event`. Do not feed events the backend also reports natively (the SDL3 manager
    /// listens to SDL itself, so `Framework` does not call this).
    void handle_event(const Event& event);

protected:
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
};

} // namespace ris
