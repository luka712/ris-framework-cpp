#pragma once

#include <cstdint>

namespace ris {

/// Usage flags of a texture, like `TextureUsage` of ris-framework-ts (same values). Combine with operator|.
///
/// OpenGL ES has no usage flags on textures, so the OpenGL ES backend ignores them, except for
/// `StorageBinding` (storage textures need OpenGL ES 3.1), which it rejects with std::domain_error.
enum class TextureUsage : uint32_t {
    None = 1,              ///< Usage is not relevant, such as in OpenGL.
    CopySrc = 2,           ///< The texture can be read from (copy source), it has no other usage.
    CopyDst = 4,           ///< The texture is the destination of copies, to upload data.
    TextureBinding = 8,    ///< The texture can be bound and read in a shader.
    StorageBinding = 16,   ///< The texture is a storage texture, written in a shader.
    RenderAttachment = 32  ///< The texture is used as a render attachment.
};

constexpr TextureUsage operator|(TextureUsage a, TextureUsage b) noexcept {
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
constexpr TextureUsage operator&(TextureUsage a, TextureUsage b) noexcept {
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
constexpr TextureUsage& operator|=(TextureUsage& a, TextureUsage b) noexcept { return a = a | b; }
/// True if every flag of `flag` is set in `flags`.
constexpr bool has_flag(TextureUsage flags, TextureUsage flag) noexcept {
    return (flags & flag) == flag;
}

} // namespace ris
