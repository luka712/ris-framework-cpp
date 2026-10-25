#pragma once

#include <cstdint>

#include "ris/rendering/buffers/vertex_format.hpp"

namespace ris {

/// One attribute of a vertex, like `VertexAttribute` of ris-framework-ts.
struct VertexAttribute {
    VertexFormat format = VertexFormat::Float32;
    uint32_t offset = 0;            ///< Byte offset from the start of the vertex.
    uint32_t shader_location = 0;   ///< `layout(location = N)` in the vertex shader.
};

} // namespace ris
