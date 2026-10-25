#include "ris/gles/utilities/gles_texture_utilities.hpp"

#include <algorithm>

#include "internal.hpp"
#include "ris/gles/utilities/debug_label_utilities.hpp"
#include "ris/gles/utilities/error_utilities.hpp"
#include "ris/gles/utilities/gles_converter.hpp"
#include "ris/rendering/texture/texture_utilities.hpp"
#include "ris/utilities/align_utilities.hpp"

using namespace ris::detail;

namespace ris::gles {

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

uint32_t GlesTextureUtilities::fullMipCount(int32_t width, int32_t height) const noexcept {
    return TextureUtilities::mipLevels(width, height);
}

GlUint GlesTextureUtilities::createTexture2D(int32_t width, int32_t height, std::span<const MipLevelData> data,
                                             TextureFormat format, bool useMipMaps, float anisotropy,
                                             std::string_view label) const {
    require_context();
    if (anisotropy > 1.0f) throw std::runtime_error("Not implemented yet.");
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument(std::string("createTexture2D: invalid size ") + std::to_string(width) + "x"
                                    + std::to_string(height));
    }
    if (converter::is_compressed_format(format)) {
        throw std::domain_error("createTexture2D: compressed formats need createCompressedTexture2D");
    }
    const GLenum internal_format = converter::convertToInternalFormat(format);
    const GLenum pixel_format = converter::convertToPixelFormat(format);
    const GLenum pixel_type = converter::convertToTextureType(format);

    const uint32_t max_levels = fullMipCount(width, height);
    if (data.size() > max_levels) {
        throw std::invalid_argument(std::string("createTexture2D: ") + std::to_string(data.size())
                                    + " mip levels given but a " + std::to_string(width) + "x" + std::to_string(height)
                                    + " texture has only " + std::to_string(max_levels));
    }
    const bool generate = useMipMaps && data.size() <= 1;
    if (generate && is_depth_format(format)) {
        throw std::invalid_argument("createTexture2D: mip maps cannot be generated for a depth format");
    }
    const uint32_t levels = generate ? max_levels : static_cast<uint32_t>(std::max<size_t>(1, data.size()));

    for (size_t i = 0; i < data.size(); ++i) {
        const size_t lw = static_cast<size_t>(std::max(1, width >> i));
        const size_t lh = static_cast<size_t>(std::max(1, height >> i));
        if (!data[i].empty() && data[i].size() < lw * lh * kBytesPerPixel) {
            throw std::invalid_argument(std::string("createTexture2D: mip level ") + std::to_string(i) + " needs "
                                        + std::to_string(lw * lh * kBytesPerPixel) + " bytes, got "
                                        + std::to_string(data[i].size()));
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

    error_utilities::check_error("createTexture2D");

    guard.released = true;
    debug_label_utilities::set_label(debug_label_utilities::ObjectKind::Texture, guard.id, label);
    return guard.id;
}

GlUint GlesTextureUtilities::createTexture2D(int32_t width, int32_t height, const void* pixels, TextureFormat format,
                                             bool useMipMaps, float anisotropy, std::string_view label) const {
    if (!pixels || width <= 0 || height <= 0) {
        return createTexture2D(width, height, std::span<const MipLevelData>{}, format, useMipMaps, anisotropy, label);
    }
    const MipLevelData level0(static_cast<const uint8_t*>(pixels),
                              static_cast<size_t>(width) * static_cast<size_t>(height) * kBytesPerPixel);
    return createTexture2D(width, height, std::span<const MipLevelData>(&level0, 1), format, useMipMaps, anisotropy,
                             label);
}

GlUint GlesTextureUtilities::createCompressedTexture2D(int32_t width, int32_t height, int32_t blockWidth,
                                                       int32_t blockHeight, std::span<const MipLevelData> data,
                                                       TextureFormat format, float anisotropy,
                                                       std::string_view label) const {
    require_context();
    if (anisotropy > 1.0f) throw std::runtime_error("Not implemented yet.");
    if (width <= 0 || height <= 0 || blockWidth <= 0 || blockHeight <= 0) {
        throw std::invalid_argument(std::string("createCompressedTexture2D: invalid size ") + std::to_string(width)
                                    + "x" + std::to_string(height) + " or block " + std::to_string(blockWidth) + "x"
                                    + std::to_string(blockHeight));
    }
    if (!converter::is_compressed_format(format)) {
        throw std::domain_error("createCompressedTexture2D: format is not compressed");
    }
    if (data.empty() || data.size() > fullMipCount(width, height)) {
        throw std::invalid_argument(std::string("createCompressedTexture2D: ") + std::to_string(data.size())
                                    + " mip levels given for a " + std::to_string(width) + "x" + std::to_string(height)
                                    + " texture");
    }
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i].empty()) {
            throw std::invalid_argument(std::string("createCompressedTexture2D: mip level ") + std::to_string(i)
                                        + " is empty");
        }
    }
    const GLenum internal_format = converter::convertToInternalFormat(format);
    const bool use_tex_storage = width % blockWidth == 0 && height % blockHeight == 0;

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
                                                                                    static_cast<size_t>(blockWidth)));
            const auto aligned_height = static_cast<GLsizei>(align_utilities::align(static_cast<size_t>(level_height),
                                                                                     static_cast<size_t>(blockHeight)));
            glCompressedTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(i), internal_format, aligned_width, aligned_height, 0,
                                   byte_size, data[i].data());
        }
        level_width = std::max(1, level_width / 2);
        level_height = std::max(1, level_height / 2);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    error_utilities::check_error("createCompressedTexture2D");

    guard.released = true;
    debug_label_utilities::set_label(debug_label_utilities::ObjectKind::Texture, guard.id, label);
    return guard.id;
}

void GlesTextureUtilities::deleteTexture(GlUint& texture) const noexcept {
    if (texture != 0 && has_context()) {
        const GLuint id = texture;
        glDeleteTextures(1, &id);
    }
    texture = 0;
}

} // namespace ris::gles
