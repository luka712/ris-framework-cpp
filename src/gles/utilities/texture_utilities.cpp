#include "ris/gles/utilities/texture_utilities.hpp"

#include <algorithm>

#include "internal.hpp"
#include "ris/gles/utilities/anisotropic_filter_utilities.hpp"
#include "ris/gles/utilities/debug_label_utilities.hpp"
#include "ris/gles/utilities/error_utilities.hpp"
#include "ris/gles/utilities/gles_converter.hpp"
#include "ris/utilities/align_utilities.hpp"

using namespace ris::detail;

namespace ris::gles::texture_utilities {

namespace {

constexpr size_t kBytesPerPixel = 4;   // every uncompressed format that has a GL mapping is 32 bit

bool is_depth_format(TextureFormat f) { return f == TextureFormat::Depth32Float || f == TextureFormat::Depth24Stencil8; }

/// Deletes the texture if the scope is left by an exception, and unbinds GL_TEXTURE_2D.
struct TextureGuard {
    GLuint id = 0;
    bool released = false;
    ~TextureGuard() {
        if (released) return;
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &id);
    }
};

/// Restores GL_UNPACK_ALIGNMENT.
struct UnpackAlignmentScope {
    GLint previous = 4;
    UnpackAlignmentScope() {
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &previous);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }
    ~UnpackAlignmentScope() { glPixelStorei(GL_UNPACK_ALIGNMENT, previous); }
};

} // namespace

uint32_t full_mip_count(int32_t width, int32_t height) noexcept {
    uint32_t levels = 1;
    int32_t size = std::max(width, height);
    while (size > 1) {
        size >>= 1;
        ++levels;
    }
    return levels;
}

GlUint create_texture_2d(int32_t width, int32_t height, std::span<const MipLevelData> data, TextureFormat format,
                         bool use_mip_maps, float anisotropy, std::string_view label) {
    require_context();
    if (width <= 0 || height <= 0) {
        throw_error(Result::InvalidArgument, "create_texture_2d: invalid size %dx%d", width, height);
    }
    if (converter::is_compressed_format(format)) {
        throw_error(Result::Unsupported, "create_texture_2d: compressed formats need create_compressed_texture_2d");
    }
    const GLenum internal_format = converter::convert_internal_format(format);
    const GLenum pixel_format = converter::convert_to_pixel_format(format);
    const GLenum pixel_type = converter::convert_to_texture_type(format);

    const uint32_t max_levels = full_mip_count(width, height);
    if (data.size() > max_levels) {
        throw_error(Result::InvalidArgument, "create_texture_2d: %zu mip levels given but a %dx%d texture has only %u",
                    data.size(), width, height, max_levels);
    }
    const bool generate = use_mip_maps && data.size() <= 1;
    if (generate && is_depth_format(format)) {
        throw_error(Result::InvalidArgument, "create_texture_2d: mip maps cannot be generated for a depth format");
    }
    const uint32_t levels = generate ? max_levels : static_cast<uint32_t>(std::max<size_t>(1, data.size()));

    for (size_t i = 0; i < data.size(); ++i) {
        const size_t lw = static_cast<size_t>(std::max(1, width >> i));
        const size_t lh = static_cast<size_t>(std::max(1, height >> i));
        if (!data[i].empty() && data[i].size() < lw * lh * kBytesPerPixel) {
            throw_error(Result::InvalidArgument, "create_texture_2d: mip level %zu needs %zu bytes, got %zu", i,
                        lw * lh * kBytesPerPixel, data[i].size());
        }
    }

    TextureGuard guard;
    glGenTextures(1, &guard.id);
    glBindTexture(GL_TEXTURE_2D, guard.id);
    glTexStorage2D(GL_TEXTURE_2D, static_cast<GLsizei>(levels), internal_format, width, height);
    {
        UnpackAlignmentScope alignment;
        for (size_t i = 0; i < data.size(); ++i) {
            if (data[i].empty()) continue;
            glTexSubImage2D(GL_TEXTURE_2D, static_cast<GLint>(i), 0, 0, std::max(1, width >> i), std::max(1, height >> i),
                            pixel_format, pixel_type, data[i].data());
        }
    }
    if (generate && !data.empty() && !data[0].empty()) glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (anisotropy > 1.0f) anisotropic_filter_utilities::set_anisotropy(guard.id, anisotropy);
    error_utilities::check_error("create_texture_2d");

    guard.released = true;
    debug_label_utilities::set_label(debug_label_utilities::ObjectKind::Texture, guard.id, label);
    return guard.id;
}

GlUint create_texture_2d(int32_t width, int32_t height, const void* pixels, TextureFormat format, bool use_mip_maps,
                         float anisotropy, std::string_view label) {
    if (!pixels || width <= 0 || height <= 0) {
        return create_texture_2d(width, height, std::span<const MipLevelData>{}, format, use_mip_maps, anisotropy, label);
    }
    const MipLevelData level0(static_cast<const uint8_t*>(pixels),
                              static_cast<size_t>(width) * static_cast<size_t>(height) * kBytesPerPixel);
    return create_texture_2d(width, height, std::span<const MipLevelData>(&level0, 1), format, use_mip_maps, anisotropy,
                             label);
}

GlUint create_compressed_texture_2d(int32_t width, int32_t height, int32_t block_width, int32_t block_height,
                                    std::span<const MipLevelData> data, TextureFormat format, float anisotropy,
                                    std::string_view label) {
    require_context();
    if (width <= 0 || height <= 0 || block_width <= 0 || block_height <= 0) {
        throw_error(Result::InvalidArgument, "create_compressed_texture_2d: invalid size %dx%d or block %dx%d", width,
                    height, block_width, block_height);
    }
    if (!converter::is_compressed_format(format)) {
        throw_error(Result::Unsupported, "create_compressed_texture_2d: format is not compressed");
    }
    if (data.empty() || data.size() > full_mip_count(width, height)) {
        throw_error(Result::InvalidArgument, "create_compressed_texture_2d: %zu mip levels given for a %dx%d texture",
                    data.size(), width, height);
    }
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i].empty()) {
            throw_error(Result::InvalidArgument, "create_compressed_texture_2d: mip level %zu is empty", i);
        }
    }
    const GLenum internal_format = converter::convert_internal_format(format);
    const bool use_tex_storage = width % block_width == 0 && height % block_height == 0;

    TextureGuard guard;
    glGenTextures(1, &guard.id);
    glBindTexture(GL_TEXTURE_2D, guard.id);
    if (use_tex_storage) {
        glTexStorage2D(GL_TEXTURE_2D, static_cast<GLsizei>(data.size()), internal_format, width, height);
    }
    int32_t level_width = width;
    int32_t level_height = height;
    for (size_t i = 0; i < data.size(); ++i) {
        const GLsizei byte_size = static_cast<GLsizei>(data[i].size());
        if (use_tex_storage) {
            glCompressedTexSubImage2D(GL_TEXTURE_2D, static_cast<GLint>(i), 0, 0, level_width, level_height, internal_format,
                                      byte_size, data[i].data());
        } else {
            const auto aligned_width = static_cast<GLsizei>(align_utilities::align(static_cast<size_t>(level_width),
                                                                                    static_cast<size_t>(block_width)));
            const auto aligned_height = static_cast<GLsizei>(align_utilities::align(static_cast<size_t>(level_height),
                                                                                     static_cast<size_t>(block_height)));
            glCompressedTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(i), internal_format, aligned_width, aligned_height, 0,
                                   byte_size, data[i].data());
        }
        level_width = std::max(1, level_width / 2);
        level_height = std::max(1, level_height / 2);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (anisotropy > 1.0f) anisotropic_filter_utilities::set_anisotropy(guard.id, anisotropy);
    error_utilities::check_error("create_compressed_texture_2d");

    guard.released = true;
    debug_label_utilities::set_label(debug_label_utilities::ObjectKind::Texture, guard.id, label);
    return guard.id;
}

void delete_texture(GlUint& texture) noexcept {
    if (texture != 0 && has_context()) {
        const GLuint id = texture;
        glDeleteTextures(1, &id);
    }
    texture = 0;
}

} // namespace ris::gles::texture_utilities
