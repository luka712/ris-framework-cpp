#pragma once

#include <cstdint>

#include "ris/rendering/swap_chain/surface_present_mode.hpp"

namespace ris {

/// The description of a swap chain, like `SwapChainDescriptor` of ris-framework-ts.
struct SwapChainDescriptor {
    /// The width of the back buffer in pixels, 0 (default) for the width of the window. The OpenGL ES backend
    /// ignores it: the back buffer is the window's surface and always has the size of the window.
    int32_t width = 0;
    /// The height of the back buffer in pixels, 0 (default) for the height of the window. Ignored like `width`.
    int32_t height = 0;
    /// How the swap chain presents the back buffer. Fifo (vsync on) by default.
    SurfacePresentMode desiredPresentationMode = SurfacePresentMode::Fifo;
};

} // namespace ris
