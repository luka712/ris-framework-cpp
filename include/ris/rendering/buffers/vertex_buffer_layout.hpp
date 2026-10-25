#pragma once

#include <cstdint>
#include <vector>

#include "ris/core.hpp"
#include "ris/graphics.hpp"
#include "ris/rendering/buffers/vertex_attribute.hpp"
#include "ris/rendering/buffers/vertex_step_mode.hpp"

namespace ris {

/// Describes how the vertices in a VertexBuffer are laid out, like `IVertexBufferLayout` of ris-framework-ts.
/// Plain data: it does not reference any GL object.
struct RIS_API VertexBufferLayout {
    uint32_t array_stride = 0;                       ///< Size of one vertex in bytes.
    VertexStepMode step_mode = VertexStepMode::Vertex;
    std::vector<VertexAttribute> attributes;

    /// position (vec3, location 0) + uv (vec2, location 1): 20 bytes.
    static VertexBufferLayout create_float3_float2();
    /// position (vec3, 0) + colour (vec4, 1) + uv (vec2, 2): 36 bytes.
    static VertexBufferLayout create_float3_float4_float2();
    /// position (vec3, 0) + colour (vec4, 1) + uv (vec2, 2) + normal (vec3, 3): 48 bytes.
    static VertexBufferLayout create_float3_float4_float2_float3();

    /// Converts the attributes to the low level description used by VertexArrayDesc.
    /// A `Float32x16` attribute expands to four vec4 attributes on consecutive locations. The step mode
    /// becomes the divisor (Instance = 1).
    /// @param buffer_slot index into VertexArrayDesc::vertex_buffers that the attributes read from.
    /// @throws Error(InvalidArgument) if an attribute does not fit into `array_stride`.
    std::vector<VertexAttrib> to_vertex_attribs(uint32_t buffer_slot = 0) const;
};

} // namespace ris
