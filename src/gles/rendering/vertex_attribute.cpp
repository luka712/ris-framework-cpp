#include "ris/gles/rendering/vertex_attribute.hpp"

#include "internal.hpp"
#include "ris/gles/utilities/gles_converter.hpp"

using namespace ris::detail;

namespace ris::gles::vertex_attribute {

void enable(const VertexAttrib& a, uint32_t stride) {

    const GLenum type = converter::convert_attrib_type(a.type);
    const void* offset = reinterpret_cast<const void*>(static_cast<uintptr_t>(a.offset));
    if (a.as_integer && a.type != AttribType::Float) {
        glVertexAttribIPointer(a.location, static_cast<GLint>(a.components), type, static_cast<GLsizei>(stride), offset);
    } else {
        glVertexAttribPointer(a.location, static_cast<GLint>(a.components), type, a.normalized ? GL_TRUE : GL_FALSE,
                              static_cast<GLsizei>(stride), offset);
    }
    glEnableVertexAttribArray(a.location);
    glVertexAttribDivisor(a.location, a.divisor);
}

} // namespace ris::gles::vertex_attribute
