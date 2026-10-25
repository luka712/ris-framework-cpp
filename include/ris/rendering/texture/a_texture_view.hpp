#pragma once

#include "ris/core.hpp"
#include "ris/rendering/texture/texture_format.hpp"
#include "ris/rendering/texture/texture_view_dimension.hpp"

namespace ris {

/// The interface for texture views, like `ITextureView` of ris-framework-ts: a way to look at (a part of) a texture
/// in a shader. Abstract and graphics API neutral: every rendering backend provides one implementation
/// (GlesTextureView for OpenGL ES). Created by ATexture2D::createView(), or owned by the texture as its default
/// view (ATexture2D::getDefaultTextureView()).
///
/// A view must not outlive its texture. There is no `dispose()` as in TypeScript (`IDisposable`): the destructor
/// releases whatever the view holds.
class RIS_API ATextureView {
public:
    virtual ~ATextureView() = default;

    /// The texture format of the texture view.
    virtual TextureFormat getTextureFormat() const noexcept = 0;

    /// The dimension of the texture view.
    virtual TextureViewDimension getDimension() const noexcept = 0;

    ATextureView(const ATextureView&) = delete;
    ATextureView& operator=(const ATextureView&) = delete;

protected:
    ATextureView() = default;
};

} // namespace ris
