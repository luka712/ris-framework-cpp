#pragma once

#include <string>

#include "ris/rendering/texture/texture_format.hpp"
#include "ris/rendering/texture/texture_view_dimension.hpp"

namespace ris {

/// The descriptor for a texture view, like `TextureViewDescriptor` of ris-framework-ts. It contains the parameters
/// that are used to create a texture view with ATexture2D::createView().
struct TextureViewDescriptor {
    /// The label of the texture view. Empty for none (`undefined` in TypeScript).
    std::string label;

    /// The dimension of the texture view. It specifies the type of the texture view and how it will be used in
    /// shaders. `Dimension2D` by default.
    TextureViewDimension dimension = TextureViewDimension::Dimension2D;

    /// The texture format of the texture view. `Undefined` (default) uses the texture view format of the texture
    /// (ATexture2D::getTextureViewFormat()). Deviation: the TypeScript default is `RGBA_8_UNORM`, which would be
    /// wrong for every other texture format.
    TextureFormat textureFormat = TextureFormat::Undefined;
};

} // namespace ris
