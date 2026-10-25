#pragma once

#include "ris/core.hpp"
#include "ris/gles/gles_types.hpp"
#include "ris/rendering/buffers/vertex_buffer_layout.hpp"

/// Applying a VertexBufferLayout to the current vertex array, the OpenGL ES side of `WebGLVertexBufferLayout`
/// of ris-framework-ts. They need the window's GL context.
namespace ris::gles::vertex_buffer_layout {

/// Binds `buffer` to `GL_ARRAY_BUFFER` and sets up and enables all attributes of the layout on the
/// currently bound vertex array (`initialize` of `WebGLVertexBufferLayout`). `GL_ARRAY_BUFFER` is
/// unbound afterwards.
/// @param buffer the vertex buffer name (VertexBuffer::gl_handle()).
/// @param layout the layout, converted with VertexBufferLayout::to_vertex_attribs().
/// @throws Error(InvalidArgument) if the layout is invalid or does not fit its stride.
RIS_API void initialize(GlUint buffer, const VertexBufferLayout& layout);

} // namespace ris::gles::vertex_buffer_layout
