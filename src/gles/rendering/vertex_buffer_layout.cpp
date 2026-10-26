#include "ris/gles/rendering/vertex_buffer_layout.hpp"

#include "internal.hpp"
#include "ris/gles/rendering/vertex_attribute.hpp"
#include "ris/rendering/buffers/vertex_format.hpp"

using namespace ris::detail;

namespace ris::gles::vertex_buffer_layout {

std::vector<VertexAttrib> to_vertex_attribs(const VertexBufferLayout& layout, uint32_t buffer_slot) {
    layout.validate();
    std::vector<VertexAttrib> out;
    out.reserve(layout.attributes.size());
    for (const VertexAttribute& a : layout.attributes) {
        // a mat4 is four vec4 columns on consecutive locations, the others are a single attribute
        const uint32_t columns = a.format == VertexFormat::Float32x16 ? 4 : 1;
        for (uint32_t c = 0; c < columns; ++c) {
            VertexAttrib v;
            v.location = a.shader_location + c;
            v.components = columns == 4 ? 4 : vertex_format_byte_size(a.format) / 4u;
            v.type = AttribType::Float;
            v.offset = a.offset + c * 16;
            v.buffer_slot = buffer_slot;
            v.divisor = layout.step_mode == VertexStepMode::Instance ? 1u : 0u;
            out.push_back(v);
        }
    }
    return out;
}

void initialize(uint32_t buffer, const VertexBufferLayout& layout) {

    const std::vector<VertexAttrib> attribs = to_vertex_attribs(layout, 0);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    for (const VertexAttrib& a : attribs) vertex_attribute::enable(a, layout.array_stride);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace ris::gles::vertex_buffer_layout
