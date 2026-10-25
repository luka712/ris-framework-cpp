#pragma once

#include <cstdint>

namespace ris {

/// The presentation mode of a swap chain, like `SurfacePresentMode` of ris-framework-ts.
enum class SurfacePresentMode : uint32_t {
    Immediate = 0,   ///< "vsync off": the frame is presented at once, without waiting for the screen refresh.
    Fifo = 1,        ///< "vsync on": frames are presented at the screen refresh.
    Mailbox = 2      ///< "fast vsync": like Fifo, but the newest queued frame is shown. OpenGL ES has no such mode and uses Fifo.
};

} // namespace ris
