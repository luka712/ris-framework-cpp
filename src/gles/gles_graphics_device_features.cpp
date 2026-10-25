#include "ris/gles/gles_graphics_device_features.hpp"
#include <algorithm>
#include <string>
#include <vector>
#include "ris/gles/utilities/gles_utilities.hpp"

namespace ris::gles {
    GlesGraphicsDeviceFeatures::GlesGraphicsDeviceFeatures() {
        std::vector<std::string> extensions = GlesUtilities::context().getExtensions();

        m_s3tc = std::ranges::find(extensions, "GL_EXT_texture_compression_s3tc") != extensions.end()
                 || std::ranges::find(extensions, "GL_ANGLE_texture_compression_dxt5") != extensions.end();
        m_bc = std::ranges::find(extensions, "GL_EXT_texture_compression_bptc") != extensions.end();
        m_etc2 = true; // core in OpenGL ES 3.0 (has_extension() above has already checked that there is a context)
        m_astc = std::ranges::find(extensions,"GL_KHR_texture_compression_astc_ldr") != extensions.end();
        m_pvrtc = std::ranges::find(extensions,"GL_IMG_texture_compression_pvrtc") != extensions.end();
    }
} // namespace ris::gles
