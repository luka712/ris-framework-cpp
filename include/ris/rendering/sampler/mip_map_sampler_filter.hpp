#pragma once

#include <cstdint>

namespace ris {

/// How a sampler selects between mip levels, like `MipMapSamplerFilter` of ris-framework-ts.
enum class MipMapSamplerFilter : uint32_t {
    None = 0,      ///< Mip mapping is disabled, level 0 is always used.
    Nearest = 1,   ///< The nearest mip level is used.
    Linear = 2     ///< The two nearest mip levels are blended.
};

} // namespace ris
