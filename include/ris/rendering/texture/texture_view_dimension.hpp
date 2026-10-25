#pragma once

#include <cstdint>

namespace ris {

/// The dimension of a texture view, like `TextureViewDimension` of ris-framework-ts (same values, without
/// `DIMENSIONFORCE32`). The 2D texture of the OpenGL ES backend only supports `Dimension2D`.
enum class TextureViewDimension : uint32_t {
    Undefined = 0,
    Dimension1D = 1,
    /// Texture view dimension for 2D textures, including multisampled 2D textures.
    Dimension2D = 2,
    Dimension2DArray = 3,
    DimensionCube = 4,
    DimensionCubeArray = 5,
    Dimension3D = 6
};

} // namespace ris
