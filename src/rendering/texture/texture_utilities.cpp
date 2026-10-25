#include "ris/rendering/texture/texture_utilities.hpp"

#include <algorithm>

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

} // namespace ris
