#include "ris/graphics.hpp"

#include <algorithm>
#include <cstring>
#include <ios>
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
#include "ris/gles/texture/gles_texture_factory.hpp"

using namespace ris::detail;

namespace ris {

namespace {

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
        throw std::invalid_argument(std::string("Texture::from_rgba8: invalid size ") + std::to_string(width) + "x"
                                    + std::to_string(height));
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
        throw std::ios_base::failure(std::string("could not load image '") + path + "'");
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



Texture Texture::from_ktx2_memory(const void* data, size_t size) {
    require_context();
    if (!data || size == 0) {
        throw std::invalid_argument("Texture::from_ktx2_memory: empty data");
    }
    KtxHandle handle;
    const KTX_error_code rc = ktxTexture2_CreateFromMemory(static_cast<const ktx_uint8_t*>(data), size,
                                                           KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &handle.tex);
    if (rc != KTX_SUCCESS) {
        throw std::runtime_error(std::string("could not parse KTX2 data: ") + ktxErrorString(rc));
    }
    const std::unique_ptr<Texture> texture(gles::GlesTextureFactory().createFromKtx2(handle.tex));
    return std::move(*texture);
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
