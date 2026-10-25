#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "ris/core.hpp"
#include "ris/gles/gles_types.hpp"
#include "ris/rendering/texture/texture_format.hpp"

namespace ris::gles {

/// Creating 2D textures, like `WebGlTextureUtilities` of ris-framework-ts. Reached through
/// `GlesUtilities::texture()`. The methods need the window's GL context and throw std::logic_error without one.
///
/// Textures are created with immutable storage (`glTexStorage2D`). The sampler state is left at the GL
/// defaults, as in the TypeScript version: set it with Texture::set_sampler(), with a sampler object
/// (sampler_utilities) or, for mip mapped textures, note that the default minification filter needs mips.
/// The textures are plain GL names owned by the caller: delete them with deleteTexture().
class RIS_API GlesTextureUtilities {
public:
    /// The pixels of one mip level: a view on bytes owned by the caller. An empty view means "leave this level
    /// uninitialised", like an empty array element in the TypeScript version.
    using MipLevelData = std::span<const uint8_t>;

    /// Creates a 2D texture. `createTexture2D` of ris-framework-ts.
    ///
    /// - `data` holds the mip levels: `data[0]` is the base level, `data[i]` has size `max(1, width >> i)` by
    ///   `max(1, height >> i)`. An empty `data` creates an uninitialised texture with one level.
    /// - With `useMipMaps` and at most one level of data, the full chain is generated from level 0
    ///   (`glGenerateMipmap`). If more levels are given they are used as they are.
    /// - Pixels are tightly packed (`GL_UNPACK_ALIGNMENT` is set to 1 during the upload and restored).
    /// - `anisotropy` above 1 is not implemented and throws `std::runtime_error("Not implemented yet.")`, like the
    ///   TypeScript version. Use a sampler (`SamplerDescriptor::maxAnisotropy`) for anisotropic filtering.
    /// - `GL_TEXTURE_2D` is unbound afterwards.
    ///
    /// @param width        width of level 0 in pixels, greater than zero.
    /// @param height       height of level 0 in pixels, greater than zero.
    /// @param data         pixel data per mip level, may be empty.
    /// @param format       an uncompressed format (Rgba8Unorm, Rgba8UnormSrgb, Depth32Float, Depth24Stencil8).
    /// @param useMipMaps   generate mip maps when only level 0 is supplied.
    /// @param anisotropy   must be 1 (or less): a larger value throws, see above.
    /// @param label        optional debug label, see debug_label_utilities.
    /// @return the texture name, to be deleted with deleteTexture().
    /// @throws std::runtime_error("Not implemented yet.") for an anisotropy above 1; std::invalid_argument for a bad size, more levels than the size has, a level that is too small,
    ///         or mip maps for a depth format; std::domain_error for compressed or unknown formats;
    ///         std::runtime_error if GL reports an error (the texture is deleted then).
    GlUint createTexture2D(int32_t width, int32_t height, std::span<const MipLevelData> data = {},
                           TextureFormat format = TextureFormat::Rgba8Unorm, bool useMipMaps = false,
                           float anisotropy = 1.0f, std::string_view label = {}) const;

    /// Creates a 2D texture from a single block of pixels (level 0), see the other overload. `pixels` may be
    /// null for an uninitialised texture. Level 0 has `width * height * 4` bytes.
    GlUint createTexture2D(int32_t width, int32_t height, const void* pixels,
                           TextureFormat format = TextureFormat::Rgba8Unorm, bool useMipMaps = false,
                           float anisotropy = 1.0f, std::string_view label = {}) const;

    /// Creates a compressed 2D texture. `createCompressedTexture2D` of ris-framework-ts.
    ///
    /// If the size is a multiple of the block size the texture gets immutable storage and every level is
    /// uploaded with `glCompressedTexSubImage2D`; otherwise every level is uploaded with
    /// `glCompressedTexImage2D` at its size rounded up to whole blocks. Wrap modes are set to
    /// `GL_CLAMP_TO_EDGE`. `GL_TEXTURE_2D` is unbound afterwards.
    ///
    /// @param width        width of level 0 in pixels, greater than zero.
    /// @param height       height of level 0 in pixels, greater than zero.
    /// @param blockWidth   width of a compression block in pixels (4 for BC / ETC2 / ASTC 4x4).
    /// @param blockHeight  height of a compression block in pixels.
    /// @param data         the compressed bytes of each level, at least one. Levels must not be empty.
    /// @param format       a compressed format, see converter::is_compressed_format. The GL extension that provides
    ///                     it must be available (GlesUtilities::context().isExtensionSupported), the function does
    ///                     not check that.
    /// @param anisotropy   must be 1 (or less): a larger value throws `std::runtime_error("Not implemented yet.")`.
    /// @param label        optional debug label.
    /// @return the texture name, to be deleted with deleteTexture().
    /// @throws std::runtime_error("Not implemented yet.") for an anisotropy above 1; std::invalid_argument,
    ///         std::domain_error for an uncompressed format, std::runtime_error if GL
    ///         reports an error (for example the format is not available; the texture is deleted then).
    GlUint createCompressedTexture2D(int32_t width, int32_t height, int32_t blockWidth, int32_t blockHeight,
                                     std::span<const MipLevelData> data,
                                     TextureFormat format = TextureFormat::Etc2Rgba8Unorm,
                                     float anisotropy = 1.0f, std::string_view label = {}) const;

    /// The number of levels of a full mip chain for a texture of this size: `floor(log2(max(w, h))) + 1`.
    uint32_t fullMipCount(int32_t width, int32_t height) const noexcept;

    /// Deletes a texture and sets the name to 0. Does nothing for 0, or when the window (and context) is gone.
    void deleteTexture(GlUint& texture) const noexcept;
};

} // namespace ris::gles
