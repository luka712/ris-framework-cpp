#pragma once

#include <cstdint>
#include <vector>

#include "ris/core.hpp"
#include "ris/rendering/buffers/vertex_attribute.hpp"
#include "ris/rendering/buffers/vertex_step_mode.hpp"

namespace ris {

/// Describes how the vertices in an AVertexBuffer are laid out, like `IVertexBufferLayout` of ris-framework-ts.
/// Plain, graphics API neutral data: it does not reference any GPU object. Each backend converts it to its own
/// description (OpenGL ES: gles::vertex_buffer_layout::to_vertex_attribs).
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

    /// Checks that every attribute has a known format and fits into `array_stride`. Backends call this before
    /// they convert the layout, so all of them reject the same layouts.
    /// @throws Error(InvalidArgument) if an attribute does not fit into `array_stride`.
    void validate() const;
};

} // namespace ris
