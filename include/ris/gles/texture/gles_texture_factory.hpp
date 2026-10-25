#pragma once

#include <memory>
#include <optional>
#include <span>
#include <string>

#include "ris/gles/texture/gles_texture2d.hpp"
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
/// The textures are GlesTexture2D objects (the return types are covariant), with GL's default sampler state (linear,
/// clamped, mip filtering when they have mips). `getTextureFormat()` of a KTX2 texture is the matching TextureFormat
/// (Rgba8Unorm, Rgba8UnormSrgb, Astc4x4Rgba, Etc2Rgba8Unorm) or Undefined for the formats TextureFormat does not describe.
class RIS_API GlesTextureFactory : public ATextureFactory {
public:
    GlesTextureFactory() = default;

    GlesTexture2D* createEmpty(
        int32_t width, int32_t height, std::optional<Color> defaultColor = std::nullopt,
        TextureUsage textureUsage = TextureUsage::CopyDst | TextureUsage::TextureBinding,
        TextureFormat textureFormat = TextureFormat::Undefined, std::string label = {}) const override;

    GlesTexture2D* create(
        int32_t width, int32_t height, std::span<const uint8_t> data, int32_t channels = 4, std::string label = {},
        TextureUsage usage = TextureUsage::CopyDst | TextureUsage::TextureBinding,
        TextureFormat textureFormat = TextureFormat::Undefined, bool generateMipMaps = false) const override;

    GlesTexture2D* createFromKtx2(ktxTexture2* ktxTexture,
                                  const TextureDescriptor& descriptor = {}) const override;
};

} // namespace ris::gles
