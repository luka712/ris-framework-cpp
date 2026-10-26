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

/// A framework with a 64x64 window and its OpenGL ES context current (the graphics device creates the context).
class HiddenWindow : public ris::Framework {
public:
    HiddenWindow() : ris::Framework(configuration()) {}

private:
    static RisFrameworkConfiguration configuration() {
        RisFrameworkConfiguration configuration = ris_createFrameworkConfiguration();
        configuration.backBufferWidth = 64;
        configuration.backBufferHeight = 64;
        return configuration;
    }
};

} // namespace ris_test
