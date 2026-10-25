#pragma once

#include <cstdint>

#include "ris/core.hpp"
#include "ris/rendering/texture/texture_format.hpp"

namespace ris {

/// Graphics API neutral helpers about textures and their formats, `TextureUtilities` of ris-framework-ts.
class RIS_API TextureUtilities {
public:
    /// Whether the format is block compressed (BC3, BC7, ETC2, ASTC).
    static bool isCompressedTextureFormat(TextureFormat format) noexcept;

    /// The number of levels of a full mip chain for a texture of this size: `floor(log2(max(width, height))) + 1`,
    /// at least 1.
    static uint32_t mipLevels(int32_t width, int32_t height) noexcept;

    /// The rough size in video memory of one mip level, in bytes: whole blocks for compressed formats (every
    /// supported one has 4x4 blocks of 16 bytes), `width * height * bytes per pixel` otherwise. 0 for `Undefined`
    /// or a size that is not positive.
    static uint64_t getVRamSize(TextureFormat format, int32_t width, int32_t height) noexcept;
};

} // namespace ris
