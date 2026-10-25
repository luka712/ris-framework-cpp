#pragma once

#include <string>

#include "ris/rendering/texture/texture_format.hpp"
#include "ris/rendering/texture/texture_usage.hpp"

namespace ris {

/// The options of a texture, like `TextureDescriptor` of ris-framework-ts. The factory functions that take
/// the size and the pixels as separate arguments do not need it; it is used by `ATextureFactory::createFromKtx2`.
///
/// `width`, `height`, `data` and `blockSize` of the TypeScript class are not here: they describe the pixels,
/// and a KTX2 file already carries them.
struct TextureDescriptor {
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
