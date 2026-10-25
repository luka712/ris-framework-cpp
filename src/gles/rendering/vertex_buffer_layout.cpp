#include "ris/gles/rendering/vertex_buffer_layout.hpp"

#include "internal.hpp"
#include "ris/gles/rendering/vertex_attribute.hpp"

using namespace ris::detail;

namespace ris::gles::vertex_buffer_layout {

void initialize(GlUint buffer, const VertexBufferLayout& layout) {
    require_context();
    const std::vector<VertexAttrib> attribs = layout.to_vertex_attribs(0);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    for (const VertexAttrib& a : attribs) vertex_attribute::enable(a, layout.array_stride);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace ris::gles::vertex_buffer_layout
