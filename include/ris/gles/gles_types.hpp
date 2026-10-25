#pragma once

#include <cstdint>

namespace ris::gles {

/// An OpenGL ES enumerant (`GLenum`). The public headers do not include the OpenGL ES headers, so the
/// GL types are spelled as plain integers, with the same size and signedness as in `<GLES3/gl3.h>`.
using GlEnum = uint32_t;
/// An OpenGL ES object name (`GLuint`): buffer, texture, shader, ... 0 is "no object".
using GlUint = uint32_t;

} // namespace ris::gles
