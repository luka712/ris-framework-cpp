#pragma once

#include <cstdint>

namespace ris {

/// Usage flags of a buffer, like `BufferUsage` of ris-framework-ts. Combine with operator|.
///
/// (The low level `ris::BufferUsage` of graphics.hpp is the update frequency hint of the OpenGL ES layer, a
/// different thing.) Which flags a backend can honour is up to the backend: OpenGL ES 3.0 only supports
/// `Vertex`, `Index`, `Uniform` and `CopyDst` and rejects the others with Result::Unsupported.
enum class BufferUsageFlags : uint32_t {
    None = 0,       ///< No usage defined.
    Vertex = 1,     ///< Vertex buffer of a draw operation.
    Index = 2,      ///< Index buffer of a draw operation.
    Uniform = 4,    ///< Uniform buffer of a draw operation.
    Storage = 8,    ///< Storage buffer. Backends without storage buffers (OpenGL ES 3.0) throw Result::Unsupported.
    MapRead = 16,   ///< Buffer can be read back by the CPU. Backends without it (OpenGL ES 3.0) throw Result::Unsupported.
    CopyDst = 32    ///< Buffer is written after creation (update()). Backends may choose a dynamic memory hint for it.
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
