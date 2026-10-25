#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "ris/gles/gles_types.hpp"
#include "ris/gles/texture/gles_texture_view.hpp"
#include "ris/rendering/texture/a_texture2D.hpp"
#include "ris/rendering/texture/texture_descriptor.hpp"

namespace ris {
class Framework;      // see ris/framework.hpp
struct SamplerDesc;   // see ris/graphics.hpp
class RenderTarget;   // see ris/graphics.hpp
} // namespace ris

namespace ris::gles {

class GlesTextureFactory;   // see ris/gles/texture/gles_texture_factory.hpp

/// The OpenGL ES implementation of ATexture2D, like `WebGlTexture2D` of ris-framework-ts: owns a GL 2D texture
/// object. Needs the window's GL context for its whole life (create, bind, destroy). Usually created by
/// GlesTextureFactory (`Framework::textureFactory()`), which returns it as ATexture2D. `ris::Texture`
/// (ris/graphics.hpp) is another name for this class.
///
/// Two ways to get one: construct it from a TextureDescriptor (size, pixels, format, ...), which only stores the
/// description (state Created), then call `initialize()`, which creates the GL texture, the size and mip level
/// numbers and the default texture view (state Initialized), like `WebGlTexture2D`; or use the factory / `from_*`
/// functions, which do both. `dispose()` deletes the GL texture and raises the disposed event; the destructor
/// calls it.
///
/// Move-only: the moved-from texture is empty (state Disposed, no GL object, no listeners, no event) and must
/// not be used again; the default view and the listeners move to the new texture.
///
/// `getTextureViewFormat()` is always the texture format, `getHandle()` is the GL texture name.
///
/// The destructor deletes the GL texture (if the window's context still exists) and then raises the disposed
/// event, with the state Disposed.
///
/// The names `from_rgba8`, `from_image_file`, `from_ktx2_memory`, `set_sampler`, `bind`, `width`, `height`,
/// `mip_levels` and `gl_handle` are the original API of `ris::Texture`, kept so that the existing code (samples, C
/// API, sprite batch, materials, content manager) does not change.
class RIS_API GlesTexture2D : public ATexture2D {
public:
    /// Describes a texture; nothing is created on the GPU until initialize().
    /// @param descriptor `width` and `height` (greater than zero), `data` (the mip levels, may be empty), the
    ///        format (`Undefined` is Rgba8Unorm, the preferred format of OpenGL ES), `blockWidth` / `blockHeight`
    ///        for compressed formats, usage, label and `generateMipmaps`. The `data` views are only read by
    ///        initialize(), the bytes must stay valid until then.
    /// @throws std::invalid_argument for a width or height that is not greater than zero.
    explicit GlesTexture2D(const TextureDescriptor& descriptor);

    /// The texture of a framework that is used where a texture is expected but none is given: 1x1, white
    /// (255, 255, 255, 255), label "RisDefaultFilledTexture". One per framework, so a new framework (and GL context)
    /// never gets the texture of an old one; it is created on first use and again if it was disposed.
    /// @return the texture, owned by this class (do not delete it, it is disposed by disposeDefault() at the latest
    ///         when the framework is destroyed). Never null.
    static GlesTexture2D* getOrCreateDefault(Framework& framework);

    /// Disposes and forgets the default texture of a framework. Called from the destructor of Framework. Does
    /// nothing if there is none.
    static void disposeDefault(Framework& framework);

    /// Creates an RGBA8 texture from pixels.
    /// @param pixels        width*height RGBA8, may be null (uninitialised).
    /// @param srgb          the data is sRGB encoded (format Rgba8UnormSrgb instead of Rgba8Unorm).
    /// @param usage         the usage flags (ignored by OpenGL ES, but reported).
    /// @param label         optional debug label.
    /// @throws std::invalid_argument for a bad size, std::logic_error without a window.
    static GlesTexture2D from_rgba8(int32_t width, int32_t height, const void* pixels, bool generate_mipmaps = true,
                                    bool srgb = false,
                                    TextureUsage usage = TextureUsage::CopyDst | TextureUsage::TextureBinding,
                                    const std::string& label = {});
    /// Loads PNG / JPG / BMP / TGA ... (stb_image through ris-ktx2) as an RGBA8 texture.
    static GlesTexture2D from_image_file(const std::string& path, bool generate_mipmaps = true, bool srgb = true,
                                         bool flip_vertically = false);
    /// Creates a texture from the bytes of a KTX2 file, see ATextureFactory::createFromKtx2().
    static GlesTexture2D from_ktx2_memory(const void* data, size_t size);

    /// Wraps an existing GL texture object and takes ownership of it (it is deleted with the texture). The
    /// returned texture is initialized.
    /// @param glHandle   the GL texture name.
    /// @param mipLevels  the number of levels the texture has.
    /// @param sizeBytes  the size in RAM, 0 computes it for RGBA8 with all levels.
    static GlesTexture2D adopt(GlUint glHandle, int32_t width, int32_t height, uint32_t mipLevels,
                               TextureFormat format = TextureFormat::Rgba8Unorm,
                               TextureUsage usage = TextureUsage::CopyDst | TextureUsage::TextureBinding,
                               std::string label = {}, uint64_t sizeBytes = 0);

    ~GlesTexture2D() override;
    GlesTexture2D(GlesTexture2D&& other) noexcept;
    GlesTexture2D& operator=(GlesTexture2D&& other) noexcept;

    // ---- ATexture2D ----
    const ATextureView& getDefaultTextureView() const override;
    TextureUsage getTextureUsage() const noexcept override { return textureUsage_; }
    TextureFormat getTextureFormat() const noexcept override { return textureFormat_; }
    TextureFormat getTextureViewFormat() const noexcept override { return textureFormat_; }
    uint32_t getId() const noexcept override { return id_; }
    uint64_t getHandle() const noexcept override { return glHandle_; }
    const std::string& getLabel() const noexcept override { return label_; }
    State getTextureState() const noexcept override { return textureState_; }
    uint64_t getSize() const noexcept override { return size_; }
    int32_t getWidth() const noexcept override { return width_; }
    int32_t getHeight() const noexcept override { return height_; }
    uint32_t getMipLevels() const noexcept override { return mipLevels_; }
    uint32_t addOnDisposedListener(std::function<void(ATexture2D&)> event) override;
    bool removeOnDisposedListener(uint32_t id) override;
    GlesTextureView* createView(const TextureViewDescriptor& descriptor = {}) override;
    void initialize() override;
    void dispose() noexcept override;

    // ---- OpenGL ES ----
    /// The GL texture name, `glTexture` of `WebGlTexture2D`, for interop with native code. Owned by this object. 0
    /// before initialize() and once disposed.
    GlUint getGlTexture() const noexcept { return glHandle_; }
    /// The same as getGlTexture().
    GlUint getGlHandle() const noexcept { return glHandle_; }

    /// Binds the texture to a texture unit (`glActiveTexture` + `glBindTexture`).
    /// @throws std::logic_error without a window.
    void bind(uint32_t unit = 0) const;

    /// Sets the sampler state stored in the texture (`glTexParameteri`), see ris::SamplerDesc.
    void set_sampler(const SamplerDesc& sampler);

    int32_t width() const noexcept { return width_; }
    int32_t height() const noexcept { return height_; }
    uint32_t mip_levels() const noexcept { return mipLevels_; }
    /// The OpenGL texture name, for interop with native code.
    uint32_t gl_handle() const noexcept { return glHandle_; }

private:
    friend class ris::RenderTarget;   // default constructs the colour texture, assigns it later

    GlesTexture2D() = default;   // empty, like a moved-from texture
    GlesTexture2D(GlUint glHandle, int32_t width, int32_t height, uint32_t mipLevels, TextureFormat format,
                  TextureUsage usage, std::string label, uint64_t sizeBytes);

    /// Creates the GL texture from the description (the part of initialize() for textures made from a descriptor).
    void createGlTexture();

    static uint32_t s_ids;
    static uint32_t s_listenerIds;

    uint32_t id_ = 0;
    GlUint glHandle_ = 0;
    int32_t width_ = 0;
    int32_t height_ = 0;
    uint32_t mipLevels_ = 1;
    TextureFormat textureFormat_ = TextureFormat::Undefined;
    TextureUsage textureUsage_ = TextureUsage::None;
    std::string label_;
    uint64_t size_ = 0;
    State textureState_ = State::Disposed;
    // The description of a texture made from a descriptor, used (and the data forgotten) by initialize().
    std::vector<std::span<const uint8_t>> data_;
    int32_t blockWidth_ = 4;
    int32_t blockHeight_ = 4;
    bool generateMipmaps_ = false;
    std::unique_ptr<GlesTextureView> defaultView_;
    std::map<uint32_t, std::function<void(ATexture2D&)>> onDisposedListeners_;
};

} // namespace ris::gles
