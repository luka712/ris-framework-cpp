#include "ris/rendering/buffers/vertex_buffer_layout.hpp"

#include <string>

// Graphics API neutral: includes no backend and no graphics API header.

namespace ris {

// ---- VertexBufferLayout ---------------------------------------------------------------------

VertexBufferLayout VertexBufferLayout::create_float3_float2() {
    return {20, VertexStepMode::Vertex,
            {{VertexFormat::Float32x3, 0, 0}, {VertexFormat::Float32x2, 12, 1}}};
}

VertexBufferLayout VertexBufferLayout::create_float3_float4_float2() {
    return {36, VertexStepMode::Vertex,
            {{VertexFormat::Float32x3, 0, 0}, {VertexFormat::Float32x4, 12, 1}, {VertexFormat::Float32x2, 28, 2}}};
}

VertexBufferLayout VertexBufferLayout::create_float3_float4_float2_float3() {
    return {48, VertexStepMode::Vertex,
            {{VertexFormat::Float32x3, 0, 0},
             {VertexFormat::Float32x4, 12, 1},
             {VertexFormat::Float32x2, 28, 2},
             {VertexFormat::Float32x3, 36, 3}}};
}

void VertexBufferLayout::validate() const {
    for (const VertexAttribute& a : attributes) {
        const uint32_t size = vertex_format_byte_size(a.format);
        if (size == 0 || a.offset > array_stride || size > array_stride - a.offset) {
            throw Error(Result::InvalidArgument,
                        "VertexBufferLayout: attribute at location " + std::to_string(a.shader_location) +
                            " (offset " + std::to_string(a.offset) + ", " + std::to_string(size) +
                            " bytes) does not fit the stride " + std::to_string(array_stride));
        }
    }
}

} // namespace ris
