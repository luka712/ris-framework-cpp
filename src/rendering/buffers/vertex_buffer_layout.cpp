#include "ris/rendering/buffers/vertex_buffer_layout.hpp"

#include "internal.hpp"

using namespace ris::detail;

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

std::vector<VertexAttrib> VertexBufferLayout::to_vertex_attribs(uint32_t buffer_slot) const {
    std::vector<VertexAttrib> out;
    out.reserve(attributes.size());
    for (const VertexAttribute& a : attributes) {
        const uint32_t size = vertex_format_byte_size(a.format);
        if (size == 0 || a.offset > array_stride || size > array_stride - a.offset) {
            throw_error(Result::InvalidArgument,
                        "VertexBufferLayout: attribute at location %u (offset %u, %u bytes) does not fit the stride %u",
                        a.shader_location, a.offset, size, array_stride);
        }
        // a mat4 is four vec4 columns on consecutive locations, the others are a single attribute
        const uint32_t columns = a.format == VertexFormat::Float32x16 ? 4 : 1;
        for (uint32_t c = 0; c < columns; ++c) {
            VertexAttrib v;
            v.location = a.shader_location + c;
            v.components = columns == 4 ? 4 : vertex_format_byte_size(a.format) / 4u;
            v.type = AttribType::Float;
            v.offset = a.offset + c * 16;
            v.buffer_slot = buffer_slot;
            v.divisor = step_mode == VertexStepMode::Instance ? 1u : 0u;
            out.push_back(v);
        }
    }
    return out;
}

} // namespace ris
