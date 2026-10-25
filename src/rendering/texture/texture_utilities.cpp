#include "ris/rendering/texture/texture_utilities.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

// ris-ktx2 installs its headers to <prefix>/include/ris_ktx2/, a source build exposes them directly.
#if __has_include(<ris_ktx2/ris_ktx.hpp>)
#include <ris_ktx2/ris_ktx.hpp>
#else
#include <ris_ktx.hpp>
#endif

namespace ris {

bool TextureUtilities::isCompressedTextureFormat(TextureFormat format) noexcept {
    switch (format) {
        case TextureFormat::Bc3RgbaUnorm:
        case TextureFormat::Bc7RgbaUnorm:
        case TextureFormat::Etc2Rgba8Unorm:
        case TextureFormat::Astc4x4Rgba: return true;
        default: return false;
    }
}

uint32_t TextureUtilities::mipLevels(int32_t width, int32_t height) noexcept {
    uint32_t levels = 1;
    int32_t size = std::max(width, height);
    while (size > 1) {
        size >>= 1;
        ++levels;
    }
    return levels;
}

uint64_t TextureUtilities::getVRamSize(TextureFormat format, int32_t width, int32_t height) noexcept {
    if (width <= 0 || height <= 0) return 0;
    const uint64_t w = static_cast<uint64_t>(width);
    const uint64_t h = static_cast<uint64_t>(height);
    if (isCompressedTextureFormat(format)) {
        return ((w + 3) / 4) * ((h + 3) / 4) * 16;
    }
    switch (format) {
        case TextureFormat::Rgba8Unorm:
        case TextureFormat::Rgba8UnormSrgb:
        case TextureFormat::Depth24Stencil8:
        case TextureFormat::Depth32Float:
        case TextureFormat::Red32Float:
        case TextureFormat::Red32Typeless: return w * h * 4;
        default: return 0;
    }
}

int32_t TextureUtilities::getBlockWidth(TextureFormat format) noexcept {
    return isCompressedTextureFormat(format) ? 4 : 1;
}

int32_t TextureUtilities::getBlockHeight(TextureFormat format) noexcept {
    return isCompressedTextureFormat(format) ? 4 : 1;
}

uint32_t TextureUtilities::convertTextureFormatToKtxTranscodeFormat(TextureFormat format) {
    switch (format) {
        case TextureFormat::Bc7RgbaUnorm: return KTX_TTF_BC7_RGBA;
        case TextureFormat::Bc3RgbaUnorm: return KTX_TTF_BC3_RGBA;
        case TextureFormat::Etc2Rgba8Unorm: return KTX_TTF_ETC2_RGBA;
        case TextureFormat::Astc4x4Rgba: return KTX_TTF_ASTC_4x4_RGBA;
        case TextureFormat::Rgba8Unorm:
        case TextureFormat::Rgba8UnormSrgb: return KTX_TTF_RGBA32;
        default:
            throw std::domain_error(std::string("texture format ") + std::to_string(static_cast<unsigned>(format))
                                    + " has no KTX2 transcode format");
    }
}

uint32_t TextureUtilities::convertKtxTranscodeFormatToVkFormat(uint32_t transcodeFormat) {
    switch (static_cast<ktx_transcode_fmt_e>(transcodeFormat)) {
        case KTX_TTF_BC7_RGBA: return VK_FORMAT_BC7_UNORM_BLOCK;
        case KTX_TTF_BC3_RGBA: return VK_FORMAT_BC3_UNORM_BLOCK;
        case KTX_TTF_ETC2_RGBA: return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
        case KTX_TTF_ASTC_4x4_RGBA: return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        case KTX_TTF_RGBA32: return VK_FORMAT_R8G8B8A8_UNORM;
        default:
            throw std::domain_error(std::string("KTX2 transcode format ") + std::to_string(transcodeFormat)
                                    + " is not supported");
    }
}

TextureFormat TextureUtilities::convertVkFormatToTextureFormat(uint32_t vkFormat) {
    switch (static_cast<VkFormat>(vkFormat)) {
        case VK_FORMAT_R8G8B8A8_UNORM: return TextureFormat::Rgba8Unorm;
        case VK_FORMAT_R8G8B8A8_SRGB: return TextureFormat::Rgba8UnormSrgb;
        case VK_FORMAT_BC7_UNORM_BLOCK: return TextureFormat::Bc7RgbaUnorm;
        case VK_FORMAT_BC3_UNORM_BLOCK: return TextureFormat::Bc3RgbaUnorm;
        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return TextureFormat::Etc2Rgba8Unorm;
        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: return TextureFormat::Astc4x4Rgba;
        default:
            throw std::domain_error(std::string("VkFormat ") + std::to_string(vkFormat)
                                    + " has no matching TextureFormat");
    }
}

} // namespace ris
