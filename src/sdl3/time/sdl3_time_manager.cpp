#include "ris/sdl3/time/sdl3_time_manager.hpp"

#include "ris/sdl3/time/sdl3_clock.hpp"

namespace ris::sdl3 {

Sdl3TimeManager::Sdl3TimeManager() : TimeManagerBase(Sdl3Clock::shared()) {}

} // namespace ris::sdl3
