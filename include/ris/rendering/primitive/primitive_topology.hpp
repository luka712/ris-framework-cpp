#pragma once

#include <cstdint>

namespace ris {

/// How vertices are assembled to primitives, like `PrimitiveTopology` of ris-framework-ts. (The older
/// `ris::Topology` of graphics.hpp is the OpenGL ES draw call enum, it also has a triangle fan.)
enum class PrimitiveTopology : uint32_t {
    LineList = 0,       ///< Every pair of vertices is a line.
    LineStrip = 1,      ///< Every vertex after the first forms a line with the previous vertex.
    PointList = 2,      ///< Every vertex is a point.
    TriangleList = 3,   ///< Every three vertices are a triangle.
    TriangleStrip = 4   ///< Every vertex after the first two forms a triangle with the previous two.
};

} // namespace ris
