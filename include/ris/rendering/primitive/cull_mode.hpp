#pragma once

#include <cstdint>

namespace ris {

/// The face culling mode, like `CullMode` of ris-framework-ts.
enum class CullMode : uint32_t {
    None = 0,    ///< Culling is disabled: front and back faces are rendered.
    Back = 1,    ///< Back faces are culled, only front faces are rendered.
    Front = 2    ///< Front faces are culled, only back faces are rendered.
};

} // namespace ris
