#include "ris/gles/gles_graphics_device_features.hpp"

#include "ris/gles/utilities/extension_utilities.hpp"

namespace ris::gles {

GlesGraphicsDeviceFeatures::GlesGraphicsDeviceFeatures() {
    using extension_utilities::has_extension;
    s3tc_ = has_extension("GL_EXT_texture_compression_s3tc") || has_extension("GL_ANGLE_texture_compression_dxt5");
    bc_ = s3tc_ && has_extension("GL_EXT_texture_compression_bptc");
    etc2_ = true; // core in OpenGL ES 3.0 (has_extension() above has already checked that there is a context)
    astc_ = has_extension("GL_KHR_texture_compression_astc_ldr");
    pvrtc_ = has_extension("GL_IMG_texture_compression_pvrtc");
}

} // namespace ris::gles
