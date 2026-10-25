#include "ris/rendering/buffers/make_vertex_array_desc.hpp"

#include "internal.hpp"

using namespace ris::detail;

namespace ris {

// ---- helpers --------------------------------------------------------------------------------

VertexArrayDesc make_vertex_array_desc(const VertexBuffer& vertex_buffer, const VertexBufferLayout& layout,
                                       const IndexBuffer* index_buffer) {
    if (layout.array_stride != vertex_buffer.byte_stride()) {
        throw_error(Result::InvalidArgument, "make_vertex_array_desc: layout stride %u differs from the buffer stride %u",
                    layout.array_stride, vertex_buffer.byte_stride());
    }
    VertexArrayDesc desc;
    desc.attribs = layout.to_vertex_attribs(0);
    desc.vertex_buffers[0] = vertex_buffer.binding();
    if (index_buffer) {
        desc.index_buffer = &index_buffer->buffer();
        desc.index_type = index_buffer->index_type();
    }
    return desc;
}

} // namespace ris
