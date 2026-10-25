#include "ris/gles/texture/gles_texture2d.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <ios>
#include <stdexcept>
#include <utility>
#include <vector>

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
#include "ris/graphics.hpp"
#include "ris/rendering/texture/texture_utilities.hpp"

using namespace ris::detail;

namespace ris::gles {

uint32_t GlesTexture2D::s_ids = 0;
uint32_t GlesTexture2D::s_listenerIds = 0;

namespace {

void apply_default_sampler(GLuint tex, uint32_t levels) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

/// The rough size in RAM of an RGBA8 texture with `levels` levels.
uint64_t rgba8_size(int32_t width, int32_t height, uint32_t levels) {
    uint64_t size = 0;
    for (uint32_t level = 0; level < levels; ++level) {
        size += static_cast<uint64_t>(std::max(1, width >> level)) * static_cast<uint64_t>(std::max(1, height >> level)) * 4;
    }
    return size;
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

// ---- creation -------------------------------------------------------------------------------

GlesTexture2D::GlesTexture2D(GlUint glHandle, int32_t width, int32_t height, uint32_t mipLevels, TextureFormat format,
                             TextureUsage usage, std::string label, uint64_t sizeBytes)
    : id_(++s_ids),
      glHandle_(glHandle),
      width_(width),
      height_(height),
      mipLevels_(mipLevels),
      textureFormat_(format),
      textureUsage_(usage),
      label_(std::move(label)),
      size_(sizeBytes),
      textureState_(State::Created) {}

GlesTexture2D::GlesTexture2D(const TextureDescriptor& descriptor)
    : id_(++s_ids),
      width_(descriptor.width),
      height_(descriptor.height),
      textureFormat_(descriptor.textureFormat == TextureFormat::Undefined ? TextureFormat::Rgba8Unorm
                                                                          : descriptor.textureFormat),
      textureUsage_(descriptor.textureUsage),
      label_(descriptor.label),
      textureState_(State::Created),
      data_(descriptor.data),
      blockWidth_(descriptor.blockWidth),
      blockHeight_(descriptor.blockHeight),
      generateMipmaps_(descriptor.generateMipmaps) {
    if (width_ <= 0 || height_ <= 0) {
        throw std::invalid_argument(std::string("GlesTexture2D: invalid size ") + std::to_string(width_) + "x"
                                    + std::to_string(height_));
    }
}

namespace {

/// One default texture per framework, owned here.
std::map<Framework*, std::unique_ptr<GlesTexture2D>>& defaultTextures() {
    static std::map<Framework*, std::unique_ptr<GlesTexture2D>> textures;
    return textures;
}

} // namespace

GlesTexture2D* GlesTexture2D::getOrCreateDefault(Framework& framework) {
    auto& textures = defaultTextures();
    auto it = textures.find(&framework);
    if (it == textures.end() || it->second->getTextureState() == State::Disposed) {
        const uint8_t white[4] = {255, 255, 255, 255};
        TextureDescriptor descriptor;
        descriptor.width = 1;
        descriptor.height = 1;
        descriptor.data.emplace_back(white, 4);
        descriptor.textureFormat = TextureFormat::Rgba8Unorm;   // the preferred format of OpenGL ES
        descriptor.label = "RisDefaultFilledTexture";
        auto texture = std::make_unique<GlesTexture2D>(descriptor);
        texture->initialize();
        textures[&framework] = std::move(texture);
        it = textures.find(&framework);
    }
    return it->second.get();
}

void GlesTexture2D::disposeDefault(Framework& framework) {
    auto& textures = defaultTextures();
    const auto it = textures.find(&framework);
    if (it == textures.end()) return;
    const std::unique_ptr<GlesTexture2D> texture = std::move(it->second);
    textures.erase(it);
    texture->dispose();
}

GlesTexture2D GlesTexture2D::adopt(GlUint glHandle, int32_t width, int32_t height, uint32_t mipLevels,
                                   TextureFormat format, TextureUsage usage, std::string label, uint64_t sizeBytes) {
    GlesTexture2D texture(glHandle, width, height, mipLevels, format, usage, std::move(label),
                          sizeBytes != 0 ? sizeBytes : rgba8_size(width, height, mipLevels));
    texture.initialize();
    return texture;
}

GlesTexture2D GlesTexture2D::from_rgba8(int32_t width, int32_t height, const void* pixels, bool generate_mipmaps,
                                        bool srgb, TextureUsage usage, const std::string& label) {
    require_context();
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument(std::string("Texture::from_rgba8: invalid size ") + std::to_string(width) + "x"
                                    + std::to_string(height));
    }
    TextureDescriptor descriptor;
    descriptor.width = width;
    descriptor.height = height;
    if (pixels) {
        descriptor.data.emplace_back(static_cast<const uint8_t*>(pixels),
                                     static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
    }
    descriptor.textureFormat = srgb ? TextureFormat::Rgba8UnormSrgb : TextureFormat::Rgba8Unorm;
    descriptor.textureUsage = usage;
    descriptor.label = label;
    descriptor.generateMipmaps = generate_mipmaps;
    GlesTexture2D texture(descriptor);
    texture.initialize();
    return texture;
}

GlesTexture2D GlesTexture2D::from_image_file(const std::string& path, bool generate_mipmaps, bool srgb,
                                             bool flip_vertically) {
    require_context();
    ris_stbi_set_flip_vertically_on_load(flip_vertically);
    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = ris_stbi_load(path.c_str(), &w, &h, &channels, 4);
    ris_stbi_set_flip_vertically_on_load(false);
    if (!pixels) {
        throw std::ios_base::failure(std::string("could not load image '") + path + "'");
    }
    try {
        GlesTexture2D t = from_rgba8(w, h, pixels, generate_mipmaps, srgb);
        ris_stbi_image_free(pixels);
        return t;
    } catch (...) {
        ris_stbi_image_free(pixels);
        throw;
    }
}

GlesTexture2D GlesTexture2D::from_ktx2_memory(const void* data, size_t size) {
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
    const std::unique_ptr<GlesTexture2D> texture(GlesTextureFactory().createFromKtx2(handle.tex));
    return std::move(*texture);
}

// ---- life cycle -----------------------------------------------------------------------------

GlesTexture2D::~GlesTexture2D() {
    dispose();
}

GlesTexture2D::GlesTexture2D(GlesTexture2D&& other) noexcept
    : ATexture2D(std::move(other)),
      id_(other.id_),
      glHandle_(std::exchange(other.glHandle_, 0)),
      width_(other.width_),
      height_(other.height_),
      mipLevels_(other.mipLevels_),
      textureFormat_(other.textureFormat_),
      textureUsage_(other.textureUsage_),
      label_(std::move(other.label_)),
      size_(other.size_),
      textureState_(std::exchange(other.textureState_, State::Disposed)),
      data_(std::move(other.data_)),
      blockWidth_(other.blockWidth_),
      blockHeight_(other.blockHeight_),
      generateMipmaps_(other.generateMipmaps_),
      defaultView_(std::move(other.defaultView_)),
      onDisposedListeners_(std::move(other.onDisposedListeners_)) {
    other.onDisposedListeners_.clear();
    other.data_.clear();
    if (defaultView_) defaultView_->texture_ = this;
}

GlesTexture2D& GlesTexture2D::operator=(GlesTexture2D&& other) noexcept {
    if (this != &other) {
        dispose();
        ATexture2D::operator=(std::move(other));
        id_ = other.id_;
        glHandle_ = std::exchange(other.glHandle_, 0);
        width_ = other.width_;
        height_ = other.height_;
        mipLevels_ = other.mipLevels_;
        textureFormat_ = other.textureFormat_;
        textureUsage_ = other.textureUsage_;
        label_ = std::move(other.label_);
        size_ = other.size_;
        textureState_ = std::exchange(other.textureState_, State::Disposed);
        data_ = std::move(other.data_);
        blockWidth_ = other.blockWidth_;
        blockHeight_ = other.blockHeight_;
        generateMipmaps_ = other.generateMipmaps_;
        defaultView_ = std::move(other.defaultView_);
        onDisposedListeners_ = std::move(other.onDisposedListeners_);
        other.onDisposedListeners_.clear();
        if (defaultView_) defaultView_->texture_ = this;
    }
    return *this;
}

void GlesTexture2D::dispose() noexcept {
    // An empty texture (default constructed or moved from) has nothing to release and raises no event.
    if (textureState_ == State::Disposing || textureState_ == State::Disposed) return;
    textureState_ = State::Disposing;
    if (glHandle_ != 0 && has_context()) {
        glDeleteTextures(1, &glHandle_);
    }
    glHandle_ = 0;
    data_.clear();
    textureState_ = State::Disposed;

    // Iterate a copy: a listener may remove itself (or another one) while this runs.
    const auto listeners = onDisposedListeners_;
    onDisposedListeners_.clear();
    for (const auto& entry : listeners) {
        try {
            entry.second(*this);
        } catch (const std::exception& e) {
            RIS_LOG_E("a disposed listener of texture %u threw: %s", id_, e.what());
        } catch (...) {
            RIS_LOG_E("a disposed listener of texture %u threw", id_);
        }
    }
}

void GlesTexture2D::initialize() {
    if (textureState_ == State::Initialized) {
        throw std::logic_error("Texture is already initialized.");
    }
    if (textureState_ != State::Created) {
        throw std::logic_error("Texture is already disposed.");
    }
    // A texture that wraps a GL texture (adopt) has its handle, levels and size already.
    if (glHandle_ == 0) createGlTexture();
    defaultView_ = std::make_unique<GlesTextureView>(*this, TextureViewDimension::Dimension2D, textureFormat_,
                                                     label_.empty() ? std::string() : label_ + " default view");
    textureState_ = State::Initialized;
}

void GlesTexture2D::createGlTexture() {
    const bool compressed = TextureUtilities::isCompressedTextureFormat(textureFormat_);
    // Anisotropy 0, like the TypeScript version (anisotropic filtering is a sampler setting).
    GlUint id = 0;
    if (compressed) {
        id = GlesUtilities::texture().createCompressedTexture2D(width_, height_, blockWidth_, blockHeight_, data_,
                                                                textureFormat_, 0.0f, label_);
    } else {
        id = GlesUtilities::texture().createTexture2D(width_, height_, data_, textureFormat_, generateMipmaps_, 0.0f,
                                                      label_);
    }

    // Mip levels: the whole chain when it was generated, else the levels that were given.
    const bool generated = generateMipmaps_ && !compressed && data_.size() <= 1;
    const uint32_t levels = generated ? TextureUtilities::mipLevels(width_, height_)
                                      : std::max<uint32_t>(1, static_cast<uint32_t>(data_.size()));
    uint64_t size = 0;
    int32_t width = width_;
    int32_t height = height_;
    for (uint32_t i = 0; i < levels; ++i) {
        size += TextureUtilities::getVRamSize(textureFormat_, width, height);
        // Each mip is at least 1x1.
        width = std::max(1, width / 2);
        height = std::max(1, height / 2);
    }

    apply_default_sampler(id, levels);
    glBindTexture(GL_TEXTURE_2D, 0);
    glHandle_ = id;
    mipLevels_ = levels;
    size_ = size;
    data_.clear();
}

// ---- ATexture2D -----------------------------------------------------------------------------

const ATextureView& GlesTexture2D::getDefaultTextureView() const {
    if (!defaultView_) {
        throw std::logic_error("getDefaultTextureView: the texture is not initialized");
    }
    return *defaultView_;
}

uint32_t GlesTexture2D::addOnDisposedListener(std::function<void(ATexture2D&)> event) {
    if (!event) {
        throw std::invalid_argument("addOnDisposedListener: the listener is empty");
    }
    const uint32_t id = ++s_listenerIds;
    onDisposedListeners_[id] = std::move(event);
    return id;
}

bool GlesTexture2D::removeOnDisposedListener(uint32_t id) {
    return onDisposedListeners_.erase(id) != 0;
}

GlesTextureView* GlesTexture2D::createView(const TextureViewDescriptor& descriptor) {
    if (textureState_ != State::Initialized) {
        throw std::logic_error("createView: the texture is not initialized (or already disposed)");
    }
    if (descriptor.dimension != TextureViewDimension::Dimension2D) {
        throw std::invalid_argument("createView: a 2D texture only has views of the dimension Dimension2D");
    }
    const TextureFormat format =
        descriptor.textureFormat == TextureFormat::Undefined ? getTextureViewFormat() : descriptor.textureFormat;
    if (format != getTextureViewFormat()) {
        throw std::domain_error("createView: OpenGL ES cannot give a view of another format than the texture view format");
    }
    return new GlesTextureView(*this, descriptor.dimension, format, descriptor.label);
}

// ---- OpenGL ES ------------------------------------------------------------------------------

void GlesTexture2D::set_sampler(const SamplerDesc& s) {
    require_context();
    glBindTexture(GL_TEXTURE_2D, glHandle_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(converter::convert_filter(s.min_filter)));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    s.mag_filter == Filter::Nearest ? GL_NEAREST : GL_LINEAR);   // only Nearest and Linear are valid
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(converter::convert_wrap(s.wrap_u)));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(converter::convert_wrap(s.wrap_v)));
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GlesTexture2D::bind(uint32_t unit) const {
    require_context();
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, glHandle_);
}

} // namespace ris::gles
