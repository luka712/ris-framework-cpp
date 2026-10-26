#include "../../../include/ris/gles/utilities/gles_context_utilities.hpp"

#include <GLES3/gl3.h>

#include "internal.hpp"

using namespace ris::detail;

std::string GlesContextUtilities::glString(GLenum name) {
    const GLubyte* s = glGetString(name);
    return s ? reinterpret_cast<const char*>(s) : "";
}

std::vector<std::string> GlesContextUtilities::getExtensions() {
       // throws std::logic_error without a window, like all the gles utilities
    GLint count = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &count);

    std::vector<std::string> extensions(count);

    for (GLint i = 0; i < count; i++) {
        const GLubyte* extension =
            glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i));

        if (extension) {
            const char* str = reinterpret_cast<const char*>(extension);
            extensions[i] = str;
        }
    }

    return extensions;
}

bool GlesContextUtilities::isExtensionSupported(const std::string &extension) {
    auto extensions = getExtensions();
    for (const auto& ext : extensions) {
        if (extension == ext) {
            return true;
        }
    }
    return false;
}
