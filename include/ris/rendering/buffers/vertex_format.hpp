#pragma once

#include <cstdint>

#include "ris/core.hpp"

namespace ris {

/// The format of one vertex attribute, like `VertexFormat` of ris-framework-ts. All formats are 32 bit floats.
enum class VertexFormat : uint32_t {
    Float32 = 0,     ///< float
    Float32x2 = 1,   ///< vec2
    Float32x3 = 2,   ///< vec3
    Float32x4 = 3,   ///< vec4
    Float32x16 = 4   ///< mat4, consumes four consecutive shader locations (one vec4 column each)
};

/// Size in bytes of a vertex format (4, 8, 12, 16 or 64).
RIS_API uint32_t vertex_format_byte_size(VertexFormat format) noexcept;

} // namespace ris
