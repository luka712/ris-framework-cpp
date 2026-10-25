#pragma once

#include <cstdint>

namespace ris {

/// Usage flags of a buffer, like `BufferUsage` of ris-framework-ts. Combine with operator|.
///
/// (The low level `ris::BufferUsage` of graphics.hpp is the GL update frequency hint, a different thing.)
/// Only `Vertex`, `Index`, `Uniform` and `CopyDst` can be honoured with OpenGL ES 3.0.
enum class BufferUsageFlags : uint32_t {
    None = 0,       ///< No usage defined.
    Vertex = 1,     ///< Vertex buffer of a draw operation (GL_ARRAY_BUFFER).
    Index = 2,      ///< Index buffer of a draw operation (GL_ELEMENT_ARRAY_BUFFER).
    Uniform = 4,    ///< Uniform buffer of a draw operation (GL_UNIFORM_BUFFER).
    Storage = 8,    ///< Storage buffer. Not available in OpenGL ES 3.0: creating one throws Result::Unsupported.
    MapRead = 16,   ///< Buffer can be read back by the CPU. Not available in OpenGL ES 3.0: throws Result::Unsupported.
    CopyDst = 32    ///< Buffer is written after creation (update()). Selects GL_DYNAMIC_DRAW, otherwise GL_STATIC_DRAW.
};

constexpr BufferUsageFlags operator|(BufferUsageFlags a, BufferUsageFlags b) noexcept {
    return static_cast<BufferUsageFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
constexpr BufferUsageFlags operator&(BufferUsageFlags a, BufferUsageFlags b) noexcept {
    return static_cast<BufferUsageFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
constexpr BufferUsageFlags& operator|=(BufferUsageFlags& a, BufferUsageFlags b) noexcept { return a = a | b; }
/// True if every flag of `flag` is set in `flags`.
constexpr bool has_flag(BufferUsageFlags flags, BufferUsageFlags flag) noexcept {
    return (flags & flag) == flag;
}

} // namespace ris
