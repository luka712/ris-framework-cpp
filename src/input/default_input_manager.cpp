#include "ris/input/default_input_manager.hpp"

#include "ris/sdl3/input/sdl3_input_manager.hpp"

namespace ris {

std::unique_ptr<AInputManager> create_default_input_manager() {
    return std::make_unique<sdl3::Sdl3InputManager>();
}

} // namespace ris
