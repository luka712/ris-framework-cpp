#pragma once

// Private to the library. The SDL event pump (`ris::Window::poll_event`) shows every SDL event it pulls to the
// registered taps before translating it to a `ris::Event`; the input manager is a tap. Single threaded: the
// taps are used from the thread that owns the window.

#include <SDL3/SDL.h>

#include <functional>

namespace ris::detail {


/// Registers `tap` under `owner` (any unique address). A second registration of the same owner replaces the first.
void add_event_tap(const void* owner, std::function<void(const SDL_Event&)> tap);
/// Removes the tap of `owner`, if any.
void remove_event_tap(const void* owner);
/// Called by the event pump for every SDL event.
void dispatch_event_tap(const SDL_Event& event);

} // namespace ris::detail
