#pragma once

#include <cstdint>

#include "ris/core.hpp"
#include "ris/graphics.hpp"

/// Setting up vertex attributes on the current vertex array, the OpenGL ES side of `WebGLVertexAttribute`
/// of ris-framework-ts (which is an empty class there). They need the window's GL context.
namespace ris::gles::vertex_attribute {

/// Points one attribute at the buffer that is bound to `GL_ARRAY_BUFFER` and enables it: `glVertexAttribPointer`
/// (or `glVertexAttribIPointer` for integer attributes), `glEnableVertexAttribArray` and
/// `glVertexAttribDivisor`. This is what the commented-out `initialize` of `WebGLVertexBufferLayout` does.
///
/// The vertex array and the array buffer must be bound by the caller.
/// @param attribute the attribute (location, component count, type, offset, normalisation, divisor).
/// @param stride    the size of a vertex in bytes.
RIS_API void enable(const VertexAttrib& attribute, uint32_t stride);

} // namespace ris::gles::vertex_attribute
