#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>

#include "ris/core.hpp"
#include "ris/data.hpp"
#include "ris/rendering/texture/a_texture2D.hpp"
#include "ris/rendering/texture/texture_descriptor.hpp"
#include "ris/rendering/texture/texture_format.hpp"
#include "ris/rendering/texture/texture_usage.hpp"

// Declared by ris-ktx2 (ktx.h), only passed by pointer here.
struct ktxTexture2;

namespace ris {

/// Creates textures, like `ITextureFactory` of ris-framework-ts. Abstract and graphics API neutral: every
/// rendering backend provides one implementation (GlesTextureFactory for OpenGL ES). Get the factory of the
/// framework from `Framework::textureFactory()`.
///
/// The factory is cheap. The textures it creates are ATexture2D objects, initialized (see
/// ATexture2D::initialize()), owned by the caller (the pointers are never null, free them with `delete`), do not
/// refer to the factory and may outlive it, but not the window / graphics device.
class RIS_API ATextureFactory {
public:
    virtual ~ATextureFactory() = default;

    /// Creates an empty 2D texture (`createEmpty` of ris-framework-ts).
    /// @param width          width in pixels, greater than zero.
    /// @param height         height in pixels, greater than zero.
    /// @param defaultColor   optional fill color. The color is written as it is (not converted to sRGB for
    ///                       Rgba8UnormSrgb). Without a color (or with all four components zero) no pixels are
    ///                       uploaded and the texture is transparent black (all zero). Unlike ris-framework-ts,
    ///                       opaque black is a color too and is written.
    /// @param textureUsage   usage flags, see TextureUsage.
    /// @param textureFormat  the format. `Undefined` selects the preferred format of the backend (Rgba8Unorm).
    ///                       Compressed formats cannot be created empty.
    /// @param label          optional debug label.
    /// @param useMipmap      allocate the whole mip level chain (and generate it when a color is given).
    /// @throws std::invalid_argument "Width and height must be greater than 0." for a bad size,
    ///         std::domain_error for a format or usage the backend cannot honour, std::logic_error without a
    ///         device, std::runtime_error if the backend fails.
    virtual ATexture2D* createEmpty(
        int32_t width, int32_t height, std::optional<Color> defaultColor = std::nullopt,
        TextureUsage textureUsage = TextureUsage::CopyDst | TextureUsage::TextureBinding,
        TextureFormat textureFormat = TextureFormat::Undefined, std::string label = {},
        bool useMipmap = false) const = 0;

    /// Creates a 2D texture from image data (`create` of ris-framework-ts). Replaces the `Uint8Array |
    /// HTMLImageElement` of ris-framework-ts with plain bytes: decode image files first (see
    /// ContentManager::loadTexture2D).
    /// @param width          width of the image in pixels, greater than zero.
    /// @param height         height of the image in pixels, greater than zero.
    /// @param data           the pixels, tightly packed rows from the top to the bottom, exactly
    ///                       `width * height * channels` bytes (copied). Must not be empty, use createEmpty()
    ///                       for a texture without pixels.
    /// @param channels       the number of channels (bytes) per pixel. Only 4 (RGBA8) is supported.
    /// @param label          optional debug label.
    /// @param usage          usage flags, `CopyDst | TextureBinding` by default.
    /// @param textureFormat  Rgba8Unorm (default, `Undefined` is the same) or Rgba8UnormSrgb.
    /// @param useMipMaps     generate the mip level chain.
    /// @throws std::invalid_argument "Data must not be null." for empty data, "width and height must be greater
    ///         than 0." for a bad size, "channels must be greater than 0.", or a data size mismatch,
    ///         std::domain_error for a format, channel count or usage the backend cannot honour,
    ///         std::logic_error without a device, std::runtime_error if the backend fails.
    virtual ATexture2D* create(
        int32_t width, int32_t height, std::span<const uint8_t> data, int32_t channels = 4, std::string label = {},
        TextureUsage usage = TextureUsage::CopyDst | TextureUsage::TextureBinding,
        TextureFormat textureFormat = TextureFormat::Rgba8Unorm, bool useMipMaps = false) const = 0;

    /// Creates a 2D texture from a KTX2 texture (`createFromKtx2` of ris-framework-ts). A Basis Universal texture is
    /// transcoded to the GPU format given by `descriptor.textureFormat`, or, for `Undefined`, to the best one the
    /// GPU supports (BC7, then ASTC 4x4, then BC3, then ETC2). All stored mip levels are uploaded. A texture that is
    /// not Basis Universal keeps its own format, which must be one `TextureFormat` describes (RGBA8, BC7, BC3, ETC2
    /// RGBA8, ASTC 4x4).
    /// @param ktxTexture  a KTX2 texture with its image data loaded. Not owned and not destroyed: the caller
    ///                    keeps it (and destroys it with `ris_ktxTexture2_Destroy`). It is transcoded in place
    ///                    if needed. Only plain 2D textures are supported.
    /// @param descriptor  optional options: `textureFormat`, `generateMipmaps` (single level, uncompressed data only),
    ///                    `textureUsage` and `label`. The size and the data come from the file.
    /// @throws std::invalid_argument for null, std::domain_error for cube maps, arrays, 3D textures, storage
    ///         usage and formats the GPU or the backend cannot use, std::runtime_error for transcoding / data
    ///         problems, std::logic_error without a device, std::runtime_error if the backend fails.
    virtual ATexture2D* createFromKtx2(ktxTexture2* ktxTexture,
                                       const TextureDescriptor& descriptor = {}) const = 0;

    ATextureFactory(const ATextureFactory&) = delete;
    ATextureFactory& operator=(const ATextureFactory&) = delete;

protected:
    ATextureFactory() = default;
};

} // namespace ris
