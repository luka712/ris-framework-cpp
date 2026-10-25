#include "ris/gles/texture/gles_texture_factory.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>
#include <vector>

// ris-ktx2 installs its headers to <prefix>/include/ris_ktx2/, a source build exposes them directly.
#if __has_include(<ris_ktx2/ris_ktx.hpp>)
#include <ris_ktx2/ris_ktx.hpp>
#else
#include <ris_ktx.hpp>
#endif

#include "internal.hpp"
#include "ris/gles/gles.hpp"
#include "ris/graphics.hpp"

using namespace ris::detail;

namespace ris::gles {

namespace {

constexpr GLenum kAstc4x4Unorm = 0x93B0;     // GL_COMPRESSED_RGBA_ASTC_4x4_KHR
constexpr GLenum kAstc4x4Srgb = 0x93D0;      // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR

struct GlFormat {
    GLenum internal_format = 0;
    GLenum format = 0;      // uncompressed only
    GLenum type = 0;        // uncompressed only
    bool compressed = false;
    bool needs_astc = false;
};

GlFormat map_vk_format(VkFormat vk) {
    const int v = static_cast<int>(vk);
    switch (vk) {
        case VK_FORMAT_R8G8B8A8_UNORM: return {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, false, false};
        case VK_FORMAT_R8G8B8A8_SRGB: return {GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE, false, false};
        case VK_FORMAT_R8G8B8_UNORM: return {GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, false, false};
        case VK_FORMAT_R8G8B8_SRGB: return {GL_SRGB8, GL_RGB, GL_UNSIGNED_BYTE, false, false};
        case VK_FORMAT_R8G8_UNORM: return {GL_RG8, GL_RG, GL_UNSIGNED_BYTE, false, false};
        case VK_FORMAT_R8_UNORM: return {GL_R8, GL_RED, GL_UNSIGNED_BYTE, false, false};
        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: return {0x9274, 0, 0, true, false};
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: return {0x9275, 0, 0, true, false};
        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: return {0x9276, 0, 0, true, false};
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: return {0x9277, 0, 0, true, false};
        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return {0x9278, 0, 0, true, false};
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: return {0x9279, 0, 0, true, false};
        default: break;
    }
    // ASTC 4x4 .. 12x12, in the same order in Vulkan (UNORM at 157, SRGB right after) and GL.
    if (v >= VK_FORMAT_ASTC_4x4_UNORM_BLOCK && v <= VK_FORMAT_ASTC_12x12_SRGB_BLOCK) {
        const int index = (v - VK_FORMAT_ASTC_4x4_UNORM_BLOCK) / 2;
        const bool srgb = ((v - VK_FORMAT_ASTC_4x4_UNORM_BLOCK) % 2) == 1;
        return {static_cast<GLenum>((srgb ? kAstc4x4Srgb : kAstc4x4Unorm) + index), 0, 0, true, true};
    }
    return {};
}

/// Applies the preferred format default (Rgba8Unorm) and checks that a plain RGBA8 texture can be made.
/// @return true for the sRGB variant.
bool resolve_rgba8(const char* what, TextureFormat& format, TextureUsage usage) {
    if (format == TextureFormat::Undefined) format = TextureFormat::Rgba8Unorm;
    if (format != TextureFormat::Rgba8Unorm && format != TextureFormat::Rgba8UnormSrgb) {
        throw std::domain_error(std::string(what) + ": only Rgba8Unorm and Rgba8UnormSrgb textures can be created");
    }
    if (has_flag(usage, TextureUsage::StorageBinding)) {
        throw std::domain_error(std::string(what) + ": storage textures are not supported on OpenGL ES 3.0");
    }
    return format == TextureFormat::Rgba8UnormSrgb;
}

void check_size(const char* what, int32_t width, int32_t height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument(std::string(what) + ": width and height must be greater than zero (got "
                                    + std::to_string(width) + "x" + std::to_string(height) + ")");
    }
}

uint8_t to_byte(float v) {
    return static_cast<uint8_t>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f));
}

void set_label(const Texture& texture, const std::string& label) {
    debug_label_utilities::set_label(debug_label_utilities::ObjectKind::Texture, texture.gl_handle(), label);
}

/// Picks the Basis Universal transcode target for the descriptor format.
ktx_transcode_fmt_e transcode_target(TextureFormat format, bool has_astc) {
    switch (format) {
        case TextureFormat::Undefined:
            // Prefer ASTC, then ETC2 (core in OpenGL ES 3, emulated by ANGLE where the GPU lacks it).
            // BC7 is deliberately not used: ANGLE does not expose it on every backend.
            return has_astc ? KTX_TTF_ASTC_4x4_RGBA : KTX_TTF_ETC2_RGBA;
        case TextureFormat::Astc4x4Rgba:
            if (!has_astc) {
                throw std::domain_error("createFromKtx2: Astc4x4Rgba needs GL_KHR_texture_compression_astc_ldr, which is unavailable");
            }
            return KTX_TTF_ASTC_4x4_RGBA;
        case TextureFormat::Etc2Rgba8Unorm: return KTX_TTF_ETC2_RGBA;
        case TextureFormat::Rgba8Unorm:
        case TextureFormat::Rgba8UnormSrgb: return KTX_TTF_RGBA32;
        default:
            throw std::domain_error(std::string("createFromKtx2: texture format ")
                                    + std::to_string(static_cast<unsigned>(format)) + " is not supported on OpenGL ES");
    }
}

} // namespace

Texture* GlesTextureFactory::createEmpty(int32_t width, int32_t height,
                                                         std::optional<Color> defaultColor, TextureUsage textureUsage,
                                                         TextureFormat textureFormat, std::string label) const {
    check_size("createEmpty", width, height);
    const bool srgb = resolve_rgba8("createEmpty", textureFormat, textureUsage);
    require_context();

    std::vector<uint8_t> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 4, 0);
    if (defaultColor) {
        const uint8_t rgba[4] = {to_byte(defaultColor->r), to_byte(defaultColor->g), to_byte(defaultColor->b),
                                 to_byte(defaultColor->a)};
        for (size_t i = 0; i < pixels.size(); i += 4) std::memcpy(&pixels[i], rgba, 4);
    }
    Texture* texture = new Texture(Texture::from_rgba8(width, height, pixels.data(), false, srgb));
    set_label(*texture, label);
    return texture;
}

Texture* GlesTextureFactory::create(int32_t width, int32_t height, std::span<const uint8_t> data,
                                                    int32_t channels, std::string label, TextureUsage usage,
                                                    TextureFormat textureFormat, bool generateMipMaps) const {
    check_size("create", width, height);
    if (data.empty()) {
        throw std::invalid_argument("create: data must not be empty (use createEmpty for no pixels)");
    }
    if (channels <= 0) throw std::invalid_argument("create: channels must be greater than zero");
    if (channels != 4) throw std::domain_error(std::string("create: only 4 channel (RGBA8) data is supported, got ")
                                               + std::to_string(channels));
    const bool srgb = resolve_rgba8("create", textureFormat, usage);
    const size_t expected = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(channels);
    if (data.size() != expected) {
        throw std::invalid_argument(std::string("create: ") + std::to_string(width) + "x" + std::to_string(height)
                                    + " with " + std::to_string(channels) + " channels needs "
                                    + std::to_string(expected) + " bytes of data, got " + std::to_string(data.size()));
    }
    require_context();

    Texture* texture = new Texture(Texture::from_rgba8(width, height, data.data(), generateMipMaps, srgb));
    set_label(*texture, label);
    return texture;
}

Texture* GlesTextureFactory::createFromKtx2(ktxTexture2* ktxTexture, const TextureDescriptor& descriptor) const {
    if (!ktxTexture) {
        throw std::invalid_argument("createFromKtx2: ktxTexture is null");
    }

    if (has_flag(descriptor.textureUsage, TextureUsage::StorageBinding)) {
        throw std::domain_error("createFromKtx2: storage textures are not supported on OpenGL ES 3.0");
    }
    require_context();

    // Ktx2 will have different cases, for now we consider the simple 2d case.
    if (ktxTexture->isCubemap || ktxTexture->isArray || ktxTexture->numDimensions != 2 || ktxTexture->baseDepth > 1) {
        throw std::domain_error("createFromKtx2: only plain 2D textures are supported in 0.1.0");
    }

    const bool hasAstc = gles::extension_utilities::has_extension("GL_KHR_texture_compression_astc_ldr");

    if (ris_ktxTexture2_NeedsTranscoding(ktxTexture)) {
        const ktx_transcode_fmt_e target = transcode_target(descriptor.textureFormat, hasAstc);
        KTX_error_code rc = ris_ktxTexture2_TranscodeBasis(ktxTexture, target, 0);
        if (rc != KTX_SUCCESS) {
            RIS_LOG_W("createFromKtx2: transcoding failed (%s), falling back to RGBA8", ktxErrorString(rc));
            rc = ris_ktxTexture2_TranscodeBasis(ktxTexture, KTX_TTF_RGBA32, 0);
            if (rc != KTX_SUCCESS) {
                throw std::runtime_error(std::string("createFromKtx2: transcoding failed: ") + ktxErrorString(rc));
            }
        }
    }

    const VkFormat vk = ris_ktxTexture2_GetVkFormat(ktxTexture);
    const GlFormat fmt = map_vk_format(vk);
    if (fmt.internal_format == 0) {
        throw std::domain_error(std::string("createFromKtx2: VkFormat ") + std::to_string(static_cast<int>(vk))
                                + " has no OpenGL ES 3 mapping");
    }
    if (fmt.needs_astc && !hasAstc) {
        throw std::domain_error("createFromKtx2: the texture is ASTC compressed but GL_KHR_texture_compression_astc_ldr is unavailable");
    }

    const int32_t width = static_cast<int32_t>(ris_ktxTexture2_GetWidth(ktxTexture));
    const int32_t height = static_cast<int32_t>(ris_ktxTexture2_GetHeight(ktxTexture));
    const uint32_t levels = std::max<uint32_t>(1, ris_ktxTexture2_GetNumLevels(ktxTexture));
    const uint8_t* data = ris_ktxTexture2_GetData(ktxTexture);
    if (!data || width <= 0 || height <= 0) {
        throw std::runtime_error("createFromKtx2: the texture has no image data");
    }

    // Mip maps are only generated for a single level of uncompressed data.
    const bool generate = descriptor.generateMipmaps && levels == 1 && !fmt.compressed;
    const uint32_t totalLevels = generate ? texture_utilities::full_mip_count(width, height) : levels;

    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexStorage2D(GL_TEXTURE_2D, static_cast<GLsizei>(totalLevels), fmt.internal_format, width, height);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (uint32_t level = 0; level < levels; ++level) {
        size_t offset = 0;
        const KTX_error_code rc = ris_ktxTexture2_GetImageOffset(ktxTexture, level, 0, 0, &offset);
        if (rc != KTX_SUCCESS) {
            glDeleteTextures(1, &id);
            throw std::runtime_error(std::string("createFromKtx2: image offset for level ") + std::to_string(level)
                                     + " failed: " + ktxErrorString(rc));
        }
        const int32_t lw = std::max(1, width >> level);
        const int32_t lh = std::max(1, height >> level);
        if (fmt.compressed) {
            const size_t size = ris_ktxTexture2_GetImageSize(ktxTexture, level);
            glCompressedTexSubImage2D(GL_TEXTURE_2D, static_cast<GLint>(level), 0, 0, lw, lh, fmt.internal_format,
                                      static_cast<GLsizei>(size), data + offset);
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, static_cast<GLint>(level), 0, 0, lw, lh, fmt.format, fmt.type, data + offset);
        }
    }
    if (generate) glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, totalLevels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    try {
        check_gl("KTX2 upload");
    } catch (...) {
        glDeleteTextures(1, &id);
        throw;
    }
    RIS_LOG_D("createFromKtx2: %dx%d, %u levels, VkFormat %d, GL internal format 0x%04X", width, height, totalLevels,
              static_cast<int>(vk), static_cast<unsigned>(fmt.internal_format));

    Texture* texture = new Texture(Texture::adopt(id, width, height, totalLevels));
    set_label(*texture, descriptor.label);
    return texture;
}

} // namespace ris::gles
