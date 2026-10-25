#pragma once

#include <cstdint>

namespace ris {

/// The winding order of front facing triangles, like `FrontFace` of ris-framework-ts.
enum class FrontFace : uint32_t {
    Cw = 0,    ///< Clockwise triangles are front facing.
    Ccw = 1    ///< Counter clockwise triangles are front facing (the default of the framework).
};

} // namespace ris
