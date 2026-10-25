#pragma once

#include <cstdint>

namespace ris {

/// The format of a texture or render buffer, like `TextureFormat` of ris-framework-ts.
///
/// Used by the OpenGL ES utilities in `ris/gles/` (see ris::gles::converter). Not every format can be
/// created with every utility, unsupported combinations throw std::domain_error.
enum class TextureFormat : uint32_t {
    Undefined = 0,         ///< Unknown format. Default value.
    Rgba8Unorm = 1,        ///< Red, green, blue, alpha, 8 bit unsigned normalized each.
    Rgba8UnormSrgb = 2,    ///< Same as Rgba8Unorm, but the colour is sRGB encoded.
    Depth24Stencil8 = 3,   ///< 24 bit depth and 8 bit stencil.
    Depth32Float = 4,      ///< 32 bit float depth.
    Red32Float = 5,        ///< One 32 bit float channel. No OpenGL ES mapping in 0.1.0.
    Red32Typeless = 6,     ///< One 32 bit typeless channel (Direct3D 11 only). No OpenGL ES mapping.
    Bc7RgbaUnorm = 7,      ///< BC7 compressed RGBA. Needs `GL_EXT_texture_compression_bptc`.
    Bc3RgbaUnorm = 8,      ///< BC3 (DXT5) compressed RGBA. Needs `GL_EXT_texture_compression_s3tc`.
    Etc2Rgba8Unorm = 9,    ///< ETC2 compressed RGBA (core in OpenGL ES 3).
    Astc4x4Rgba = 10       ///< ASTC 4x4 compressed RGBA. Needs `GL_KHR_texture_compression_astc_ldr`.
};

} // namespace ris
