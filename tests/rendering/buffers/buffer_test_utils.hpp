#pragma once

// Shared helpers of the buffer tests. The GL tests need a display (or Xvfb) and an EGL implementation
// like test_window.cpp.

#include <cstdlib>

#include <ris/ris.hpp>

namespace ris_test {

inline bool display_available() {
#if defined(__linux__)
    return std::getenv("DISPLAY") != nullptr || std::getenv("WAYLAND_DISPLAY") != nullptr;
#else
    return true;
#endif
}

inline ris::Window make_hidden_window() {
    ris::WindowDesc desc;
    desc.width = 64;
    desc.height = 64;
    desc.flags = ris::WindowFlags::Hidden;
    return ris::Window(desc);
}

} // namespace ris_test
