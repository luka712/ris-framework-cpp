#pragma once

#include "ris/core.hpp"
#include "ris/graphics.hpp"
#include "ris/rendering/buffers/index_buffer.hpp"
#include "ris/rendering/buffers/vertex_buffer.hpp"
#include "ris/rendering/buffers/vertex_buffer_layout.hpp"

namespace ris {

/// Builds the description of a VertexArray that reads `vertex_buffer` through `layout` (buffer slot 0)
/// and optionally draws indexed with `index_buffer`.
/// The returned description holds pointers to the buffers, so they must outlive the VertexArray created
/// from it.
/// @throws Error(InvalidArgument) if `layout.array_stride` differs from the buffer's stride or an
///         attribute does not fit into the stride.
RIS_API VertexArrayDesc make_vertex_array_desc(const VertexBuffer& vertex_buffer, const VertexBufferLayout& layout,
                                               const IndexBuffer* index_buffer = nullptr);

} // namespace ris
