#pragma once

#include <cstdint>

namespace ris {

/// The texel filter of a sampler, like `SamplerFilter` of ris-framework-ts.
/// (The older `ris::Filter` of graphics.hpp combines this with the mipmap filter.)
enum class SamplerFilter : uint32_t {
    Nearest = 0,   ///< Value of the nearest texel.
    Linear = 1     ///< Weighted average of the four nearest texels.
};

} // namespace ris
