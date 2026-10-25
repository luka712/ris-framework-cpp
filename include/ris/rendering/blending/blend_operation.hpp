#pragma once

#include <cstdint>

namespace ris {

/// How the source and destination colours are combined, like `BlendOperation` of ris-framework-ts.
enum class BlendOperation : uint32_t {
    Add = 0,               ///< source * sf + destination * df
    Subtract = 1,          ///< source * sf - destination * df
    ReverseSubtract = 2,   ///< destination * df - source * sf
    Min = 3,               ///< min(source, destination), the factors are ignored
    Max = 4                ///< max(source, destination), the factors are ignored
};

} // namespace ris
