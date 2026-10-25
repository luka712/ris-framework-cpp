#pragma once

#include <cstdint>

namespace ris {

/// The shader stage, like `ShaderStage` of ris-framework-ts. Values are flags there; OpenGL ES 3.0 has the
/// Vertex and Fragment stages only (Compute needs ES 3.1), so only those can be compiled.
enum class ShaderStage : uint32_t {
    None = 0,
    Vertex = 1,
    Fragment = 2,
    Compute = 4   ///< Not available in OpenGL ES 3.0, compiling it throws Error(Unsupported).
};

} // namespace ris
