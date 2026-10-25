#include "ris/gles/utilities/error_utilities.hpp"

#include "internal.hpp"

using namespace ris::detail;

namespace ris::gles::error_utilities {

const char* error_name(GlEnum error) noexcept {
    switch (error) {
        case GL_NO_ERROR: return "GL_NO_ERROR";
        case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
        default: return "unknown";
    }
}

void clear_errors() {
    require_context();
    while (glGetError() != GL_NO_ERROR) {
    }
}

void check_error(const char* what) {
    require_context();
    GLenum err = glGetError();
    if (err == GL_NO_ERROR) return;
    const GLenum first = err;
    while ((err = glGetError()) != GL_NO_ERROR) {
    }
    throw_error(Result::Graphics, "OpenGL ES error 0x%04X in %s", static_cast<unsigned>(first), what);
}

} // namespace ris::gles::error_utilities
