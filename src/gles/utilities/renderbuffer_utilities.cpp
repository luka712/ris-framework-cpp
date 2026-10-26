#include "ris/gles/utilities/renderbuffer_utilities.hpp"

#include "internal.hpp"
#include "ris/gles/utilities/gles_debug_utilities.hpp"
#include "ris/gles/utilities/error_utilities.hpp"
#include "ris/gles/utilities/gles_converter.hpp"

using namespace ris::detail;

namespace ris::gles::renderbuffer_utilities {

uint32_t create(TextureFormat format, int32_t width, int32_t height, std::string_view label) {

    if (width <= 0 || height <= 0) {
        throw std::invalid_argument(std::string("renderbuffer_utilities::create: invalid size ")
                                    + std::to_string(width) + "x" + std::to_string(height));
    }
    const GLenum internal_format = converter::convertToInternalFormat(format);

    GLuint rb = 0;
    glGenRenderbuffers(1, &rb);
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
    glRenderbufferStorage(GL_RENDERBUFFER, internal_format, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    try {
        error_utilities::check_error("renderbuffer_utilities::create");
    } catch (...) {
        glDeleteRenderbuffers(1, &rb);
        throw;
    }
    debug_label_utilities::set_label(debug_label_utilities::ObjectKind::Renderbuffer, rb, label);
    return rb;
}

void delete_renderbuffer(uint32_t& renderbuffer) noexcept {
    if (renderbuffer != 0 ) {
        const GLuint id = renderbuffer;
        glDeleteRenderbuffers(1, &id);
    }
    renderbuffer = 0;
}

} // namespace ris::gles::renderbuffer_utilities
