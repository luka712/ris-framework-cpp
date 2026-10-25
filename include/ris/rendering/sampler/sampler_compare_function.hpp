#pragma once

#include <cstdint>

namespace ris {

/// The comparison of a depth (shadow) sampler, like `SamplerCompareFunction` of ris-framework-ts.
enum class SamplerCompareFunction : uint32_t {
    Never = 0,       ///< "No comparison": the sampler returns the depth value itself (compare mode off).
    LessEqual = 1    ///< Passes if the reference value is less than or equal to the sampled depth.
};

} // namespace ris
