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

    /// The width in pixels of a compression block: 4 for the compressed formats, 1 for the others.
    static int32_t getBlockWidth(TextureFormat format) noexcept;

    /// The height in pixels of a compression block: 4 for the compressed formats, 1 for the others.
    static int32_t getBlockHeight(TextureFormat format) noexcept;

    // The three conversions below use plain integers for the ris-ktx2 enums, so that this header does not need the
    // ris-ktx2 headers: the transcode format is a `ktx_transcode_fmt_e` value and the Vulkan format a `VkFormat`
    // value, as ris-ktx2 declares them.

    /// The Basis Universal transcode target (`ktx_transcode_fmt_e`) that produces a texture format: BC7_RGBA, BC3_RGBA,
    /// ETC2_RGBA, ASTC_4x4_RGBA, and RGBA32 for Rgba8Unorm / Rgba8UnormSrgb.
    /// `convertTextureFormatToKtxTranscodeFormat` of ris-framework-ts.
    /// @throws std::domain_error for a format that has no transcode target (depth, Undefined, ...).
    static uint32_t convertTextureFormatToKtxTranscodeFormat(TextureFormat format);

    /// The Vulkan format (`VkFormat`) of the data a transcode target produces. `convertKtxTranscodeFormatToVkFormat`
    /// of ris-framework-ts.
    /// @throws std::domain_error for a transcode target that is not one of those of the function above.
    static uint32_t convertKtxTranscodeFormatToVkFormat(uint32_t transcodeFormat);

    /// The texture format of Vulkan format (`VkFormat`) data: RGBA8 (UNORM and SRGB), BC7, BC3, ETC2 RGBA8 and
    /// ASTC 4x4 (UNORM). `convertVkFormatToTextureFormat` of ris-framework-ts.
    /// @throws std::domain_error for a Vulkan format that `TextureFormat` does not describe.
    static TextureFormat convertVkFormatToTextureFormat(uint32_t vkFormat);
};

} // namespace ris
