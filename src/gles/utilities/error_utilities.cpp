#include "ris/gles/utilities/error_utilities.hpp"

#include <cstdio>
#include <stdexcept>
#include <string>

#include "internal.hpp"

using namespace ris::detail;

namespace ris::gles::error_utilities {

const char* error_name(uint32_t error) noexcept {
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
    char hex[16];
    std::snprintf(hex, sizeof hex, "%04X", static_cast<unsigned>(first));
    throw std::runtime_error(std::string("OpenGL ES error 0x") + hex + " in " + what);
}

} // namespace ris::gles::error_utilities
