#include "ris/gles/utilities/anisotropic_filter_utilities.hpp"

#include <algorithm>

#include "internal.hpp"
#include "ris/gles/utilities/gles_utilities.hpp"

using namespace ris::detail;

namespace ris::gles::anisotropic_filter_utilities {

namespace {
// <GLES2/gl2ext.h> constants of GL_EXT_texture_filter_anisotropic.
constexpr GLenum kTextureMaxAnisotropy = 0x84FE;
constexpr GLenum kMaxTextureMaxAnisotropy = 0x84FF;
constexpr const char* kExtension = "GL_EXT_texture_filter_anisotropic";
} // namespace

bool is_supported() { return GlesUtilities::context().isExtensionSupported(kExtension); }

float get_max_anisotropy() {
    if (!is_supported()) return 0.0f;
    GLfloat max = 0.0f;
    glGetFloatv(kMaxTextureMaxAnisotropy, &max);
    return max;
}

void set_anisotropy(GlUint texture, float anisotropy) {
    if (!is_supported()) return;
    const float level = std::clamp(anisotropy, 1.0f, std::max(1.0f, get_max_anisotropy()));
    GLint previous = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameterf(GL_TEXTURE_2D, kTextureMaxAnisotropy, level);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previous));
}

} // namespace ris::gles::anisotropic_filter_utilities
