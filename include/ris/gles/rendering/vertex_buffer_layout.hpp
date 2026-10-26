#pragma once

#include <cstdint>
#include <vector>

#include "ris/core.hpp"
#include "ris/graphics.hpp"
#include "ris/rendering/buffers/vertex_buffer_layout.hpp"

/// Applying a VertexBufferLayout to the current vertex array, the OpenGL ES side of `WebGLVertexBufferLayout`
/// of ris-framework-ts. They need the window's GL context (except to_vertex_attribs).
namespace ris::gles::vertex_buffer_layout {

/// Converts the attributes of `layout` to the low level description used by VertexArrayDesc.
/// A `Float32x16` attribute expands to four vec4 attributes on consecutive locations. The step mode becomes
/// the divisor (Instance = 1). Needs no window.
/// @param layout      the layout; VertexBufferLayout::validate() is called first.
/// @param buffer_slot index into VertexArrayDesc::vertex_buffers that the attributes read from.
/// @throws std::invalid_argument if an attribute does not fit into `layout.array_stride`.
RIS_API std::vector<VertexAttrib> to_vertex_attribs(const VertexBufferLayout& layout, uint32_t buffer_slot = 0);

/// Binds `buffer` to `GL_ARRAY_BUFFER` and sets up and enables all attributes of the layout on the
/// currently bound vertex array (`initialize` of `WebGLVertexBufferLayout`). `GL_ARRAY_BUFFER` is
/// unbound afterwards.
/// @param buffer the vertex buffer name (GlesVertexBuffer::gl_handle()).
/// @param layout the layout, converted with to_vertex_attribs().
/// @throws std::invalid_argument if the layout is invalid or does not fit its stride.
RIS_API void initialize(uint32_t buffer, const VertexBufferLayout& layout);

} // namespace ris::gles::vertex_buffer_layout
