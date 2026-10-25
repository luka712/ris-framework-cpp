#include "ris/gles/buffers/make_vertex_array_desc.hpp"

#include "ris/gles/buffers/gles_index_buffer.hpp"
#include "ris/gles/buffers/gles_vertex_buffer.hpp"
#include "ris/gles/rendering/vertex_buffer_layout.hpp"
#include "ris/rendering/buffers/buffer_validation.hpp"

namespace ris::gles {

VertexArrayDesc make_vertex_array_desc(const AVertexBuffer& vertex_buffer, const VertexBufferLayout& layout,
                                       const AIndexBuffer* index_buffer) {
    const auto* gles_vertex = dynamic_cast<const GlesVertexBuffer*>(&vertex_buffer);
    const auto* gles_index = index_buffer ? dynamic_cast<const GlesIndexBuffer*>(index_buffer) : nullptr;
    if (!gles_vertex || (index_buffer && !gles_index)) {
        throw Error(Result::InvalidArgument,
                    "make_vertex_array_desc: the buffers were not created by the OpenGL ES backend");
    }
    buffer_validation::check_layout_stride(layout, gles_vertex->byte_stride(), "make_vertex_array_desc");
    VertexArrayDesc desc;
    desc.attribs = vertex_buffer_layout::to_vertex_attribs(layout, 0);
    desc.vertex_buffers[0] = gles_vertex->binding();
    if (gles_index) {
        desc.index_buffer = &gles_index->buffer();
        desc.index_type = gles_index->index_type();
    }
    return desc;
}

} // namespace ris::gles
