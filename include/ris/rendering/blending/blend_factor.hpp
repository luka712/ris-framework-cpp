#pragma once

#include <cstdint>

namespace ris {

/// A blend factor, like `BlendFactor` of ris-framework-ts. The TypeScript enum has `One`, `SrcAlpha` and
/// `OneMinusSrcAlpha`; the others are the ones its converter has commented out.
enum class BlendFactor : uint32_t {
    One = 0,                 ///< 1
    SrcAlpha = 1,            ///< source alpha
    OneMinusSrcAlpha = 2,    ///< 1 - source alpha
    Zero = 3,                ///< 0
    SrcColor = 4,            ///< source colour
    OneMinusSrcColor = 5,    ///< 1 - source colour
    DstColor = 6,            ///< destination colour
    OneMinusDstColor = 7,    ///< 1 - destination colour
    DstAlpha = 8,            ///< destination alpha
    OneMinusDstAlpha = 9     ///< 1 - destination alpha
};

} // namespace ris
