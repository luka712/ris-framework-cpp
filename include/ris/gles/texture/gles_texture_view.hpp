#pragma once

#include <string>

#include "ris/rendering/texture/a_texture_view.hpp"

namespace ris::gles {

class GlesTexture2D;   // see ris/gles/texture/gles_texture2d.hpp

/// The OpenGL ES implementation of ATextureView. OpenGL ES 3.0 has no texture views (`glTextureView` needs OpenGL ES
/// 3.2 / an extension), so a view is a lightweight object: it refers to its texture and remembers the format and the
/// dimension, and bind() binds the texture. Owns no GL object. Created by GlesTexture2D::createView(), or owned by the
/// texture as its default view.
///
/// A view must not outlive its texture (when the texture is moved the views follow it, see GlesTexture2D).
class RIS_API GlesTextureView : public ATextureView {
public:
    /// @param texture   the texture to look at, not owned.
    /// @param dimension the dimension.
    /// @param format    the format.
    /// @param label     optional label.
    GlesTextureView(GlesTexture2D& texture, TextureViewDimension dimension, TextureFormat format,
                    std::string label = {});

    TextureFormat getTextureFormat() const noexcept override { return format_; }
    TextureViewDimension getDimension() const noexcept override { return dimension_; }

    /// The label. Empty if there is none.
    const std::string& getLabel() const noexcept { return label_; }

    /// The texture this view looks at.
    GlesTexture2D& getTexture() const noexcept { return *texture_; }

    /// Binds the texture of the view to a texture unit, like GlesTexture2D::bind().
    /// @throws std::logic_error without a window.
    void bind(uint32_t unit = 0) const;

private:
    friend class GlesTexture2D;   // follows its texture when that is moved

    GlesTexture2D* texture_;
    TextureViewDimension dimension_;
    TextureFormat format_;
    std::string label_;
};

} // namespace ris::gles
