#pragma once

#include <cstdint>

namespace ris {

/// How a vertex buffer is stepped through while drawing, like `VertexStepMode` of ris-framework-ts.
enum class VertexStepMode : uint32_t {
    Vertex = 0,    ///< One element per vertex (attribute divisor 0).
    Instance = 1   ///< One element per instance (attribute divisor 1).
};

} // namespace ris
