#pragma once

#include "ris/rendering/a_graphics_device_features.hpp"

namespace ris::gles {

/// The OpenGL ES implementation of AGraphicsDeviceFeatures, like `WebGLGraphicsDeviceFeatures` of ris-framework-ts.
/// Queries the extensions of the current context once, when it is created, so it needs the window's GL context
/// for the constructor only. The extensions (ANGLE reports the ones its backend supports):
///  - S3TC: `GL_EXT_texture_compression_s3tc` or `GL_ANGLE_texture_compression_dxt5`
///  - BC: `GL_EXT_texture_compression_bptc` (BC6H / BC7) together with S3TC
///  - ETC2: always, it is core in OpenGL ES 3.0
///  - ASTC: `GL_KHR_texture_compression_astc_ldr`
///  - PVRTC: `GL_IMG_texture_compression_pvrtc`
class RIS_API GlesGraphicsDeviceFeatures : public AGraphicsDeviceFeatures {
public:
    /// @throws std::logic_error without a window.
    GlesGraphicsDeviceFeatures();

    bool supportsTextureCompressionS3TC() const override { return s3tc_; }
    bool supportsTextureCompressionBC() const override { return bc_; }
    bool supportsTextureCompressionETC2() const override { return etc2_; }
    bool supportsTextureCompressionASTC() const override { return astc_; }
    bool supportsTextureCompressionPVRTC() const override { return pvrtc_; }

private:
    bool s3tc_ = false;
    bool bc_ = false;
    bool etc2_ = false;
    bool astc_ = false;
    bool pvrtc_ = false;
};

} // namespace ris::gles
