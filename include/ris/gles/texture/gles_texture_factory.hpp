#pragma once

#include <memory>
#include <optional>
#include <span>
#include <string>

#include "ris/gles/texture/gles_texture2d.hpp"
#include "ris/rendering/texture/a_texture_factory.hpp"

namespace ris {
class Framework;   // see ris/framework.hpp
class AGraphicsDeviceFeatures;
} // namespace ris

namespace ris::gles {

/// The OpenGL ES implementation of ATextureFactory, like `WebGlTextureFactory` of ris-framework-ts. Every function
/// describes the texture with a TextureDescriptor, constructs a GlesTexture2D from it and calls
/// GlesTexture2D::initialize(). Needs the window's GL context. See ATextureFactory for the arguments and errors of
/// the functions.
///
/// Formats: createEmpty() and create() make Rgba8Unorm and Rgba8UnormSrgb textures (`Undefined` is Rgba8Unorm, the
/// preferred format of OpenGL ES; createEmpty() also makes depth textures). createFromKtx2() with `Undefined` picks
/// BC7, ASTC 4x4, BC3 or ETC2, the first one the GPU supports (AGraphicsDeviceFeatures), or the format of the
/// descriptor (BC7, BC3, ETC2, ASTC 4x4, Rgba8Unorm / Rgba8UnormSrgb, which transcode to RGBA8). KTX2 textures
/// that are not Basis Universal keep their own format if `TextureFormat` describes it.
/// The textures are GlesTexture2D objects (the return types are covariant), with GL's default sampler state (linear,
/// clamped, mip filtering when they have mips).
class RIS_API GlesTextureFactory : public ATextureFactory {
public:
    /// A factory that asks the graphics device of the framework for the GPU features (`WebGlTextureFactory(framework)`
    /// of ris-framework-ts). The framework must outlive the factory.
    explicit GlesTextureFactory(Framework& framework);

    /// A factory without a framework (the C API, tests with a plain window): the GPU features are queried from the
    /// current GL context instead (GlesGraphicsDeviceFeatures).
    GlesTextureFactory() = default;

    GlesTexture2D* createEmpty(
        int32_t width, int32_t height, std::optional<Color> defaultColor = std::nullopt,
        TextureUsage textureUsage = TextureUsage::CopyDst | TextureUsage::TextureBinding,
        TextureFormat textureFormat = TextureFormat::Undefined, std::string label = {},
        bool useMipmap = false) const override;

    GlesTexture2D* create(
        int32_t width, int32_t height, std::span<const uint8_t> data, int32_t channels = 4, std::string label = {},
        TextureUsage usage = TextureUsage::CopyDst | TextureUsage::TextureBinding,
        TextureFormat textureFormat = TextureFormat::Rgba8Unorm, bool useMipMaps = false) const override;

    GlesTexture2D* createFromKtx2(ktxTexture2* ktxTexture,
                                  const TextureDescriptor& descriptor = {}) const override;

private:
    Framework* framework_ = nullptr;   // not owned, null without a framework
};

} // namespace ris::gles
