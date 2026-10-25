//
// Created by luka on 25.10.26.
//

#ifndef RIS_FRAMEWORK_A_TEXTURE_HPP
#define RIS_FRAMEWORK_A_TEXTURE_HPP

#include <cstdint>
#include <functional>
#include <string>

#include "ris/content/a_content.hpp"
#include "ris/core.hpp"
#include "ris/data/state.hpp"
#include "ris/rendering/texture/a_texture_view.hpp"
#include "ris/rendering/texture/texture_format.hpp"
#include "ris/rendering/texture/texture_usage.hpp"
#include "ris/rendering/texture/texture_view_descriptor.hpp"

namespace ris {

/// The texture 2D interface, like `ITexture2D` of ris-framework-ts. Abstract and graphics API neutral: every
/// rendering backend provides one implementation (GlesTexture2D for OpenGL ES). Textures are created by
/// ATextureFactory, which returns them initialized.
///
/// Rules every implementation follows:
///  - The texture is owned by the caller of the factory (free it with `delete`, before the window / graphics
///    device). The default texture view is owned by the texture, the views of createView() by the caller (free
///    them before the texture).
///  - There is no `dispose()` as in TypeScript (`IContent` / `IDisposable`): the destructor releases the GPU
///    resources and then raises the disposed event.
///  - Failures throw standard exceptions: std::logic_error for a texture in the wrong state (initializing it twice,
///    using it after it was disposed), std::invalid_argument for bad arguments, std::domain_error for values the
///    backend does not support.
class RIS_API ATexture2D : public AContent {
public:
    ~ATexture2D() override = default;

    /// The default texture view. It is created with default parameters (see TextureViewDescriptor) and can be used
    /// when no specific view is needed. Owned by the texture.
    /// @throws std::logic_error if the texture is not initialized.
    virtual const ATextureView& getDefaultTextureView() const = 0;

    /// The texture usage.
    virtual TextureUsage getTextureUsage() const noexcept = 0;

    /// The texture format. `Undefined` if the format is not described by TextureFormat (for example the RGB8 or
    /// RG8 of a KTX2 file).
    virtual TextureFormat getTextureFormat() const noexcept = 0;

    /// The texture view format. In most cases it is the same as the texture format, but in some cases it can be
    /// different. A case where it might differ is if the texture is used as a depth texture and for shader binding.
    virtual TextureFormat getTextureViewFormat() const noexcept = 0;

    /// The id of the texture. Unique and increasing, starting with 1 (the TypeScript `id`).
    virtual uint32_t getId() const noexcept = 0;

    /// The handle of the underlying graphics API texture object, for example the OpenGL ES texture name. 0 if there
    /// is none (the texture is disposed). Replaces the `handle?: any` of the TypeScript interface. The texture owns the
    /// object, do not delete it.
    virtual uint64_t getHandle() const noexcept = 0;

    /// The label. Empty if there is none (`null` in TypeScript).
    virtual const std::string& getLabel() const noexcept = 0;

    /// The current state.
    virtual State getTextureState() const noexcept = 0;

    /// The size of this texture in RAM, in bytes. Note that the size is a rough approximation (all mip levels, whole
    /// blocks for compressed formats), as the real size can depend on the driver implementation.
    virtual uint64_t getSize() const noexcept = 0;

    /// The width of the texture in pixels.
    virtual int32_t getWidth() const noexcept = 0;

    /// The height of the texture in pixels.
    virtual int32_t getHeight() const noexcept = 0;

    /// The number of mipmap levels that the texture really has (1 without mip maps).
    virtual uint32_t getMipLevels() const noexcept = 0;

    /// Adds a listener for the event that is raised when the texture is disposed, that is, from its destructor, after
    /// the GPU resources were released (the state is Disposed and getHandle() is 0 then). The texture is only
    /// accessible for reading its properties, it is being destroyed. Deviation from the TypeScript interface (which
    /// returns void and removes the listener by function): the listener gets an id, plain `std::function`s cannot be
    /// compared.
    /// @param event the listener.
    /// @return the id of the listener, for removeOnDisposedListener(). Unique and increasing, starting with 1.
    virtual uint32_t addOnDisposedListener(std::function<void(ATexture2D&)> event) = 0;

    /// Removes a listener of the event that is raised when the texture is disposed.
    /// @param id the id returned by addOnDisposedListener().
    /// @return true if the listener existed, false for an unknown id (already removed).
    virtual bool removeOnDisposedListener(uint32_t id) = 0;

    /// Creates a texture view with the specified description.
    /// @param descriptor the texture view descriptor.
    /// @return the texture view, owned by the caller (never null). It must not outlive the texture.
    /// @throws std::invalid_argument for a dimension the texture cannot have, std::domain_error for a format the
    ///         backend cannot give a view of, std::logic_error if the texture is not initialized.
    virtual ATextureView* createView(const TextureViewDescriptor& descriptor = {}) = 0;

    /// Initializes the texture: creates the default texture view. The factories of ATextureFactory return textures that
    /// are initialized already (state Initialized), so there is normally no need to call this.
    /// @throws std::logic_error if the texture is already initialized or disposed.
    virtual void initialize() = 0;

protected:
    ATexture2D() = default;
    ATexture2D(ATexture2D&&) noexcept = default;
    ATexture2D& operator=(ATexture2D&&) noexcept = default;
};

} // namespace ris

#endif //RIS_FRAMEWORK_A_TEXTURE_HPP
