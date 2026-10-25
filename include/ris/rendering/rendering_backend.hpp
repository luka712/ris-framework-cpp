#pragma once

#include <cstdint>

namespace ris {

/// The rendering API a graphics device uses, like `RenderingBackend` of ris-framework-ts (which names WebGPU and
/// WebGL, the native framework names the APIs it has backends for).
enum class RenderingBackend : uint32_t {
    Default = 0,   ///< Unknown, or chosen automatically. Also what `AGpuInfo::getUnderlyingBackend()` says when it is not detected (ANGLE's API is not reported through GL).
    OpenGLES = 1   ///< OpenGL ES 3 (through ANGLE, or a native OpenGL ES driver).
};

} // namespace ris
