#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>

#include "ris/rendering/texture/a_texture_factory.hpp"

namespace ris::gles {

/// The OpenGL ES implementation of ATextureFactory, like `WebGlTextureFactory` of ris-framework-ts. Stateless,
/// so it can be created freely; needs the window's GL context. See ATextureFactory for the arguments and errors
/// of the functions.
///
/// Supported formats are Rgba8Unorm and Rgba8UnormSrgb for createEmpty() and create() (`Undefined` is
/// Rgba8Unorm), and for createFromKtx2() `Undefined` (ASTC 4x4 if the GPU has it, else ETC2, else RGBA8), Astc4x4Rgba,
/// Etc2Rgba8Unorm and Rgba8Unorm / Rgba8UnormSrgb (both transcode to RGBA8). KTX2 textures that are not Basis
/// Universal keep their own format (RGBA8, RGB8, RG8, R8, ETC2, ASTC) and ignore `textureFormat`.
/// The textures get GL's default sampler state of Texture (linear, clamped, mip filtering when they have mips).
class RIS_API GlesTextureFactory : public ATextureFactory {
public:
    GlesTextureFactory() = default;

    Texture* createEmpty(
        int32_t width, int32_t height, std::optional<Color> defaultColor = std::nullopt,
        TextureUsage textureUsage = TextureUsage::CopyDst | TextureUsage::TextureBinding,
        TextureFormat textureFormat = TextureFormat::Undefined, std::string label = {}) const override;

    Texture* create(
        int32_t width, int32_t height, std::span<const uint8_t> data, int32_t channels = 4, std::string label = {},
        TextureUsage usage = TextureUsage::CopyDst | TextureUsage::TextureBinding,
        TextureFormat textureFormat = TextureFormat::Undefined, bool generateMipMaps = false) const override;

    Texture* createFromKtx2(ktxTexture2* ktxTexture,
                                            const TextureDescriptor& descriptor = {}) const override;
};

} // namespace ris::gles
