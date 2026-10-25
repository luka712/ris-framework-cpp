#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>

#include "ris/gles/gles_types.hpp"
#include "ris/gles/texture/gles_texture_view.hpp"
#include "ris/rendering/texture/a_texture2D.hpp"

namespace ris {
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

    // ---- OpenGL ES ----
    /// The GL texture name, for interop with native code. Owned by this object. 0 once disposed.
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

    /// Deletes the GL texture and raises the disposed event, once. Does nothing for an empty texture.
    void dispose() noexcept;

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
    std::unique_ptr<GlesTextureView> defaultView_;
    std::map<uint32_t, std::function<void(ATexture2D&)>> onDisposedListeners_;
};

} // namespace ris::gles
