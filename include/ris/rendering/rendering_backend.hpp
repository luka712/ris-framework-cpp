#pragma once

#include <cstdint>

namespace ris {

/// The rendering API a graphics device uses, like `RenderingBackend` of ris-framework-ts (which names WebGPU and
/// WebGL, the native framework names the APIs it has backends for).
enum class RenderingBackend : uint32_t {
    Default = 0,   ///< Unknown, or chosen automatically. Also what `GpuInfo::underlyingBackend` says when the API hides it.
    OpenGLES = 1   ///< OpenGL ES 3 (through ANGLE, or a native OpenGL ES driver).
};

} // namespace ris
