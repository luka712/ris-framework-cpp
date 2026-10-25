#pragma once

#include "ris/core.hpp"
#include "ris/graphics.hpp"
#include "ris/rendering/buffers/a_index_buffer.hpp"
#include "ris/rendering/buffers/a_vertex_buffer.hpp"
#include "ris/rendering/buffers/vertex_buffer_layout.hpp"

namespace ris::gles {

/// Builds the description of a VertexArray that reads `vertex_buffer` through `layout` (buffer slot 0) and
/// optionally draws indexed with `index_buffer`. This is where the abstract buffers meet the OpenGL ES vertex
/// array, so the buffers must have been created by the OpenGL ES backend (GlesBufferFactory or the Gles
/// buffer classes).
/// The returned description holds pointers to the buffers, so they must outlive the VertexArray created from it.
/// @throws std::invalid_argument if `layout.array_stride` differs from the buffer's stride, an attribute does
///         not fit into the stride, or a buffer was created by another backend.
RIS_API VertexArrayDesc make_vertex_array_desc(const AVertexBuffer& vertex_buffer, const VertexBufferLayout& layout,
                                               const AIndexBuffer* index_buffer = nullptr);

} // namespace ris::gles
