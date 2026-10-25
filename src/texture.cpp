#include "ris/graphics.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

// ris-ktx2 installs its headers to <prefix>/include/ris_ktx2/, a source build exposes them directly.
#if __has_include(<ris_ktx2/ris_ktx.hpp>)
#include <ris_ktx2/ris_ktx.hpp>
#include <ris_ktx2/ris_stb_image.h>
#else
#include <ris_ktx.hpp>
#include <ris_stb_image.h>
#endif

#include "internal.hpp"
#include "ris/gles/gles.hpp"

using namespace ris::detail;

namespace ris {

namespace {

constexpr GLenum kAstc4x4Unorm = 0x93B0;     // GL_COMPRESSED_RGBA_ASTC_4x4_KHR
constexpr GLenum kAstc4x4Srgb = 0x93D0;      // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR

void apply_default_sampler(GLuint tex, uint32_t levels) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

/// Owns a ktxTexture2 and destroys it through ris-ktx2.
struct KtxHandle {
    ktxTexture2* tex = nullptr;
    KtxHandle() = default;
    KtxHandle(const KtxHandle&) = delete;
    KtxHandle& operator=(const KtxHandle&) = delete;
    ~KtxHandle() {
        if (tex) ris_ktxTexture2_Destroy(tex);
    }
};

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

struct Uploaded {
    GLuint id;
    int32_t width, height;
    uint32_t levels;
};

Uploaded upload_ktx2(ktxTexture2* tex, const char* origin) {
    require_context();

    if (tex->isCubemap || tex->isArray || tex->numDimensions != 2 || tex->baseDepth > 1) {
        throw_error(Result::Unsupported, "KTX2 '%s': only plain 2D textures are supported in 0.1.0", origin);
    }

    if (ris_ktxTexture2_NeedsTranscoding(tex)) {
        // Prefer ASTC, then ETC2 (core in OpenGL ES 3, emulated by ANGLE where the GPU lacks it).
        // BC7 is deliberately not used: ANGLE does not expose it on every backend.
        const bool astc = gles::extension_utilities::has_extension("GL_KHR_texture_compression_astc_ldr");
        const ktx_transcode_fmt_e target = astc ? KTX_TTF_ASTC_4x4_RGBA : KTX_TTF_ETC2_RGBA;
        KTX_error_code rc = ris_ktxTexture2_TranscodeBasis(tex, target, 0);
        if (rc != KTX_SUCCESS) {
            RIS_LOG_W("KTX2 '%s': transcoding to %s failed (%s), falling back to RGBA8", origin,
                      astc ? "ASTC" : "ETC2", ktxErrorString(rc));
            rc = ris_ktxTexture2_TranscodeBasis(tex, KTX_TTF_RGBA32, 0);
            if (rc != KTX_SUCCESS) {
                throw_error(Result::Ktx, "KTX2 '%s': transcoding failed: %s", origin, ktxErrorString(rc));
            }
        }
    }

    const VkFormat vk = ris_ktxTexture2_GetVkFormat(tex);
    const GlFormat fmt = map_vk_format(vk);
    if (fmt.internal_format == 0) {
        throw_error(Result::Unsupported, "KTX2 '%s': VkFormat %d has no OpenGL ES 3 mapping", origin, static_cast<int>(vk));
    }
    if (fmt.needs_astc && !gles::extension_utilities::has_extension("GL_KHR_texture_compression_astc_ldr")) {
        throw_error(Result::Unsupported, "KTX2 '%s' is ASTC compressed but GL_KHR_texture_compression_astc_ldr is unavailable",
                    origin);
    }

    const int32_t width = static_cast<int32_t>(ris_ktxTexture2_GetWidth(tex));
    const int32_t height = static_cast<int32_t>(ris_ktxTexture2_GetHeight(tex));
    const uint32_t levels = std::max<uint32_t>(1, ris_ktxTexture2_GetNumLevels(tex));
    const uint8_t* data = ris_ktxTexture2_GetData(tex);
    if (!data || width <= 0 || height <= 0) {
        throw_error(Result::Ktx, "KTX2 '%s' has no image data", origin);
    }

    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexStorage2D(GL_TEXTURE_2D, static_cast<GLsizei>(levels), fmt.internal_format, width, height);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (uint32_t level = 0; level < levels; ++level) {
        size_t offset = 0;
        const KTX_error_code rc = ris_ktxTexture2_GetImageOffset(tex, level, 0, 0, &offset);
        if (rc != KTX_SUCCESS) {
            glDeleteTextures(1, &id);
            throw_error(Result::Ktx, "KTX2 '%s': image offset for level %u failed: %s", origin, level, ktxErrorString(rc));
        }
        const int32_t lw = std::max(1, width >> level);
        const int32_t lh = std::max(1, height >> level);
        if (fmt.compressed) {
            const size_t size = ris_ktxTexture2_GetImageSize(tex, level);
            glCompressedTexSubImage2D(GL_TEXTURE_2D, static_cast<GLint>(level), 0, 0, lw, lh, fmt.internal_format,
                                      static_cast<GLsizei>(size), data + offset);
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, static_cast<GLint>(level), 0, 0, lw, lh, fmt.format, fmt.type, data + offset);
        }
    }
    apply_default_sampler(id, levels);
    glBindTexture(GL_TEXTURE_2D, 0);

    try {
        check_gl("KTX2 upload");
    } catch (...) {
        glDeleteTextures(1, &id);
        throw;
    }
    RIS_LOG_D("KTX2 '%s': %dx%d, %u levels, VkFormat %d, GL internal format 0x%04X", origin, width, height, levels,
              static_cast<int>(vk), static_cast<unsigned>(fmt.internal_format));
    return {id, width, height, levels};
}

} // namespace

// ---- Texture --------------------------------------------------------------------------------

Texture Texture::adopt(uint32_t id, int32_t width, int32_t height, uint32_t mip_levels) {
    Texture t;
    t.id_ = id;
    t.width_ = width;
    t.height_ = height;
    t.mip_levels_ = mip_levels;
    return t;
}

Texture Texture::from_rgba8(int32_t width, int32_t height, const void* pixels, bool generate_mipmaps, bool srgb) {
    require_context();
    if (width <= 0 || height <= 0) {
        throw_error(Result::InvalidArgument, "Texture::from_rgba8: invalid size %dx%d", width, height);
    }
    const uint32_t levels = generate_mipmaps ? gles::texture_utilities::full_mip_count(width, height) : 1;
    const gles::GlUint id = gles::texture_utilities::create_texture_2d(
        width, height, pixels, srgb ? TextureFormat::Rgba8UnormSrgb : TextureFormat::Rgba8Unorm, generate_mipmaps);
    apply_default_sampler(id, levels);
    glBindTexture(GL_TEXTURE_2D, 0);
    return adopt(id, width, height, levels);
}

Texture Texture::from_image_file(const std::string& path, bool generate_mipmaps, bool srgb, bool flip_vertically) {
    require_context();
    ris_stbi_set_flip_vertically_on_load(flip_vertically);
    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = ris_stbi_load(path.c_str(), &w, &h, &channels, 4);
    ris_stbi_set_flip_vertically_on_load(false);
    if (!pixels) {
        throw_error(Result::Io, "could not load image '%s'", path.c_str());
    }
    try {
        Texture t = from_rgba8(w, h, pixels, generate_mipmaps, srgb);
        ris_stbi_image_free(pixels);
        return t;
    } catch (...) {
        ris_stbi_image_free(pixels);
        throw;
    }
}

Texture Texture::from_ktx2_file(const std::string& path) {
    require_context();
    KtxHandle handle;
    const KTX_error_code rc =
        ris_ktxTexture2_CreateFromNamedFile(path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &handle.tex);
    if (rc != KTX_SUCCESS) {
        throw_error(rc == KTX_FILE_OPEN_FAILED || rc == KTX_FILE_READ_ERROR ? Result::Io : Result::Ktx,
                    "could not load KTX2 '%s': %s", path.c_str(), ktxErrorString(rc));
    }
    const Uploaded u = upload_ktx2(handle.tex, path.c_str());
    return adopt(u.id, u.width, u.height, u.levels);
}

Texture Texture::from_ktx2_memory(const void* data, size_t size) {
    require_context();
    if (!data || size == 0) {
        throw_error(Result::InvalidArgument, "Texture::from_ktx2_memory: empty data");
    }
    KtxHandle handle;
    const KTX_error_code rc = ktxTexture2_CreateFromMemory(static_cast<const ktx_uint8_t*>(data), size,
                                                           KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &handle.tex);
    if (rc != KTX_SUCCESS) {
        throw_error(Result::Ktx, "could not parse KTX2 data: %s", ktxErrorString(rc));
    }
    const Uploaded u = upload_ktx2(handle.tex, "<memory>");
    return adopt(u.id, u.width, u.height, u.levels);
}

Texture::~Texture() {
    if (id_ != 0 && has_context()) {
        glDeleteTextures(1, &id_);
    }
}

Texture::Texture(Texture&& other) noexcept
    : id_(std::exchange(other.id_, 0)),
      width_(other.width_),
      height_(other.height_),
      mip_levels_(other.mip_levels_) {}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (id_ != 0 && has_context()) glDeleteTextures(1, &id_);
        id_ = std::exchange(other.id_, 0);
        width_ = other.width_;
        height_ = other.height_;
        mip_levels_ = other.mip_levels_;
    }
    return *this;
}

void Texture::set_sampler(const SamplerDesc& s) {
    require_context();
    glBindTexture(GL_TEXTURE_2D, id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(gles::converter::convert_filter(s.min_filter)));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    s.mag_filter == Filter::Nearest ? GL_NEAREST : GL_LINEAR);   // only Nearest and Linear are valid
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(gles::converter::convert_wrap(s.wrap_u)));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(gles::converter::convert_wrap(s.wrap_v)));
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::bind(uint32_t unit) const {
    require_context();
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id_);
}

} // namespace ris
