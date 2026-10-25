#pragma once

#include <cstdint>

namespace ris {

/// The texture filtering of the default texture sampler, like `TextureSamplerFilteringPreset` of
/// ris-framework-ts. Also used by `FrameworkConfiguration::textureFiltering`.
enum class TextureFilteringPreset : uint32_t {
    Nearest = 0,   ///< Point filtering: nearest texel for minification and magnification, no mip mapping.
    Bilinear = 1,  ///< Linear filtering, no mip interpolation.
    Trilinear = 2  ///< Linear filtering with linear mip interpolation.
};

} // namespace ris
