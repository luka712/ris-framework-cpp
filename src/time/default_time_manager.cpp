#include "ris/time/default_time_manager.hpp"

#include "ris/sdl3/time/sdl3_clock.hpp"
#include "ris/sdl3/time/sdl3_time_manager.hpp"

namespace ris {

AClock& default_clock() { return sdl3::Sdl3Clock::shared(); }

std::unique_ptr<ATimeManager> create_default_time_manager() { return std::make_unique<sdl3::Sdl3TimeManager>(); }

} // namespace ris
