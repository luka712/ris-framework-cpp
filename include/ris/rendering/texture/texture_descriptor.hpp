#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "ris/rendering/texture/texture_format.hpp"
#include "ris/rendering/texture/texture_usage.hpp"

namespace ris {

/// The options of a texture, like `TextureDescriptor` of ris-framework-ts. `ATextureFactory::createFromKtx2` only
/// uses the format, usage, label and `generateMipmaps` (a KTX2 file carries the size and the pixels itself);
/// `GlesTexture2D`'s constructor uses all of it.
struct TextureDescriptor {
    /// Width of level 0 in pixels (GlesTexture2D constructor only).
    int32_t width = 0;
    /// Height of level 0 in pixels (GlesTexture2D constructor only).
    int32_t height = 0;
    /// The pixels of every mip level, level 0 first (`data` of the TypeScript descriptor). Views on bytes owned by
    /// the caller, which must stay valid until the texture is initialized; the texture does not keep them. Empty (or
    /// an empty first view) creates an uninitialised texture. Compressed formats need every level.
    std::vector<std::span<const uint8_t>> data;
    /// Width of a compression block in pixels, compressed formats only (`blockSize.x` of the TypeScript descriptor).
    int32_t blockWidth = 4;
    /// Height of a compression block in pixels, compressed formats only (`blockSize.y`).
    int32_t blockHeight = 4;
    /// The format to use. `Undefined` (default) lets the backend choose the best one the GPU supports.
    TextureFormat textureFormat = TextureFormat::Undefined;
    /// The usage, `TextureBinding | CopyDst` by default.
    TextureUsage textureUsage = TextureUsage::TextureBinding | TextureUsage::CopyDst;
    /// Optional debug label.
    std::string label;
    /// Generate the mip levels. Only has an effect when the source has a single level and its format is not
    /// compressed.
    bool generateMipmaps = false;
};

} // namespace ris
