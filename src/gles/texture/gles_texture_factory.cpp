#include "ris/gles/texture/gles_texture_factory.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>

// ris-ktx2 installs its headers to <prefix>/include/ris_ktx2/, a source build exposes them directly.
#if __has_include(<ris_ktx2/ris_ktx.hpp>)
#include <ris_ktx2/ris_ktx.hpp>
#else
#include <ris_ktx.hpp>
#endif

#include "internal.hpp"
#include "ris/framework.hpp"
#include "ris/gles/gles_graphics_device_features.hpp"
#include "ris/rendering/texture/texture_utilities.hpp"

using namespace ris::detail;

namespace ris::gles {

namespace {

uint8_t to_byte(float v) {
    return static_cast<uint8_t>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f));
}

/// OpenGL ES 3.0 has no storage textures.
void check_usage(const char* what, TextureUsage usage) {
    if (has_flag(usage, TextureUsage::StorageBinding)) {
        throw std::domain_error(std::string(what) + ": storage textures are not supported on OpenGL ES 3.0");
    }
}

/// Whether the GPU can sample a compressed format.
bool supports_format(const AGraphicsDeviceFeatures& features, TextureFormat format) {
    switch (format) {
        case TextureFormat::Bc7RgbaUnorm: return features.supportsTextureCompressionBC();
        case TextureFormat::Bc3RgbaUnorm: return features.supportsTextureCompressionS3TC();
        case TextureFormat::Etc2Rgba8Unorm: return features.supportsTextureCompressionETC2();
        case TextureFormat::Astc4x4Rgba: return features.supportsTextureCompressionASTC();
        default: return true;
    }
}

} // namespace

GlesTextureFactory::GlesTextureFactory(Framework& framework) : framework_(&framework) {}

GlesTexture2D* GlesTextureFactory::createFromKtx2(ktxTexture2* ktxTexture, const TextureDescriptor& descriptor) const {
    if (!ktxTexture) {
        throw std::invalid_argument("createFromKtx2: ktxTexture is null");
    }
    check_usage("createFromKtx2", descriptor.textureUsage);

    // Ktx2 will have different cases, for now we consider the simple 2d case.
    if (ktxTexture->isCubemap || ktxTexture->isArray || ktxTexture->numDimensions != 2 || ktxTexture->baseDepth > 1) {
        throw std::domain_error("createFromKtx2: only plain 2D textures are supported in 0.1.0");
    }


    VkFormat vkFormat = ris_ktxTexture2_GetVkFormat(ktxTexture);
    const uint32_t numLevels = std::max<uint32_t>(1, ris_ktxTexture2_GetNumLevels(ktxTexture));
    TextureFormat textureFormat = descriptor.textureFormat;

    // The GPU features of the graphics device of the framework, or of the current context without a framework.
    std::unique_ptr<GlesGraphicsDeviceFeatures> contextFeatures;
    if (!framework_) contextFeatures = std::make_unique<GlesGraphicsDeviceFeatures>();
    const AGraphicsDeviceFeatures& gpuFeatures =
        framework_ ? framework_->graphicsDevice().getFeatures() : *contextFeatures;

    if (ris_ktxTexture2_NeedsTranscoding(ktxTexture) && vkFormat == VK_FORMAT_UNDEFINED) {
        // If the descriptor format is Undefined, the format is chosen according to the GPU features.
        if (textureFormat == TextureFormat::Undefined) {
            if (gpuFeatures.supportsTextureCompressionBC()) {
                textureFormat = TextureFormat::Bc7RgbaUnorm;
            } else if (gpuFeatures.supportsTextureCompressionASTC()) {
                textureFormat = TextureFormat::Astc4x4Rgba;
            } else if (gpuFeatures.supportsTextureCompressionS3TC()) {
                textureFormat = TextureFormat::Bc3RgbaUnorm;
            } else if (gpuFeatures.supportsTextureCompressionETC2()) {
                textureFormat = TextureFormat::Etc2Rgba8Unorm;
            } else {
                throw std::domain_error("createFromKtx2: the GPU supports no compressed texture format");
            }
        } else if (!supports_format(gpuFeatures, textureFormat)) {
            throw std::domain_error("createFromKtx2: the GPU does not support the texture format "
                                    + std::to_string(static_cast<unsigned>(textureFormat)));
        }
        const auto transcodeFormat = static_cast<ktx_transcode_fmt_e>(
            TextureUtilities::convertTextureFormatToKtxTranscodeFormat(textureFormat));

        const KTX_error_code rc = ris_ktxTexture2_TranscodeBasis(ktxTexture, transcodeFormat, KTX_TF_HIGH_QUALITY);
        if (rc != KTX_SUCCESS) {
            throw std::runtime_error(std::string("createFromKtx2: transcoding failed: ") + ktxErrorString(rc));
        }
        vkFormat = ris_ktxTexture2_GetVkFormat(ktxTexture);
    } else {
        // No transcoding path, the format of the file is the format of the texture.
        textureFormat = TextureUtilities::convertVkFormatToTextureFormat(vkFormat);
        if (!supports_format(gpuFeatures, textureFormat)) {
            throw std::domain_error("createFromKtx2: the GPU does not support the texture format of the file");
        }
    }

    const uint8_t* base = ris_ktxTexture2_GetData(ktxTexture);
    if (!base) {
        throw std::runtime_error("createFromKtx2: the texture has no image data");
    }

    // One view per level, on the data of the KTX2 texture (valid until the texture is initialized).
    TextureDescriptor textureDescriptor;
    textureDescriptor.width = static_cast<int32_t>(ris_ktxTexture2_GetWidth(ktxTexture));
    textureDescriptor.height = static_cast<int32_t>(ris_ktxTexture2_GetHeight(ktxTexture));
    for (uint32_t level = 0; level < numLevels; ++level) {
        size_t offset = 0;
        const KTX_error_code rc = ris_ktxTexture2_GetImageOffset(ktxTexture, level, 0, 0, &offset);
        if (rc != KTX_SUCCESS) {
            throw std::runtime_error(std::string("createFromKtx2: image offset for level ") + std::to_string(level)
                                     + " failed: " + ktxErrorString(rc));
        }
        textureDescriptor.data.emplace_back(base + offset, ris_ktxTexture2_GetImageSize(ktxTexture, level));
    }
    textureDescriptor.textureFormat = textureFormat;
    textureDescriptor.generateMipmaps = descriptor.generateMipmaps;
    textureDescriptor.blockWidth = TextureUtilities::getBlockWidth(textureFormat);
    textureDescriptor.blockHeight = TextureUtilities::getBlockHeight(textureFormat);
    // Not in the TypeScript version, which drops them: the usage and label of the caller.
    textureDescriptor.textureUsage = descriptor.textureUsage;
    textureDescriptor.label = descriptor.label;

    auto texture = std::make_unique<GlesTexture2D>(textureDescriptor);
    texture->initialize();
    return texture.release();
}

GlesTexture2D* GlesTextureFactory::create(int32_t width, int32_t height, std::span<const uint8_t> data,
                                          int32_t channels, std::string label, TextureUsage usage,
                                          TextureFormat textureFormat, bool useMipMaps) const {
    if (data.empty()) {
        throw std::invalid_argument("Data must not be null.");
    }
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("width and height must be greater than 0.");
    }
    if (channels <= 0) {
        throw std::invalid_argument("channels must be greater than 0.");
    }
    // Beyond the TypeScript checks: what the backend can create from this data.
    if (channels != 4) {
        throw std::domain_error("create: only 4 channel (RGBA8) data is supported, got " + std::to_string(channels));
    }
    if (textureFormat == TextureFormat::Undefined) textureFormat = TextureFormat::Rgba8Unorm;
    if (textureFormat != TextureFormat::Rgba8Unorm && textureFormat != TextureFormat::Rgba8UnormSrgb) {
        throw std::domain_error("create: only Rgba8Unorm and Rgba8UnormSrgb textures can be created from data");
    }
    check_usage("create", usage);
    const size_t expected = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(channels);
    if (data.size() != expected) {
        throw std::invalid_argument("create: " + std::to_string(width) + "x" + std::to_string(height) + " with "
                                    + std::to_string(channels) + " channels needs " + std::to_string(expected)
                                    + " bytes of data, got " + std::to_string(data.size()));
    }


    TextureDescriptor desc;
    desc.width = width;
    desc.height = height;
    desc.data.push_back(data);
    desc.textureFormat = textureFormat;
    desc.textureUsage = usage;
    desc.generateMipmaps = useMipMaps;
    desc.label = std::move(label);

    auto texture = std::make_unique<GlesTexture2D>(desc);
    texture->initialize();
    return texture.release();
}

GlesTexture2D* GlesTextureFactory::createEmpty(int32_t width, int32_t height, std::optional<Color> defaultColor,
                                               TextureUsage textureUsage, TextureFormat textureFormat,
                                               std::string label, bool useMipmap) const {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Width and height must be greater than 0.");
    }
    // The preferred texture format of OpenGL ES.
    if (textureFormat == TextureFormat::Undefined) textureFormat = TextureFormat::Rgba8Unorm;
    if (TextureUtilities::isCompressedTextureFormat(textureFormat)) {
        throw std::domain_error("createEmpty: compressed textures cannot be created empty");
    }
    check_usage("createEmpty", textureUsage);


    // Fast path: no CPU side buffer and no upload when the texture is transparent black, which is what a new texture is.
    // (The TypeScript version also takes the opaque Color.black() here, which makes it transparent.)
    if (!defaultColor || *defaultColor == Color::transparent()) {
        TextureDescriptor desc;
        desc.width = width;
        desc.height = height;
        desc.textureUsage = textureUsage;
        desc.textureFormat = textureFormat;
        desc.label = std::move(label);
        desc.generateMipmaps = useMipmap;

        auto texture = std::make_unique<GlesTexture2D>(desc);
        texture->initialize();
        return texture.release();
    }

    // Fill the CPU side pixels in RGBA order.
    std::vector<uint8_t> bytes(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
    const uint8_t rgba[4] = {to_byte(defaultColor->r), to_byte(defaultColor->g), to_byte(defaultColor->b),
                             to_byte(defaultColor->a)};
    for (size_t i = 0; i < bytes.size(); i += 4) std::memcpy(&bytes[i], rgba, 4);

    return create(width, height, bytes, 4, std::move(label), textureUsage, textureFormat, useMipmap);
}

} // namespace ris::gles
