#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "ris/core.hpp"
#include "ris/graphics.hpp"
#include "ris/rendering/buffers/buffer_usage.hpp"

namespace ris::gles {

/// The storage shared by GlesVertexBuffer, GlesIndexBuffer and GlesUniformBuffer: one OpenGL ES buffer object
/// (owned through the low level ris::Buffer) plus the usage flags and the debug label. It is not an ABuffer
/// itself; the three concrete buffer classes hold one and forward the ABuffer functions to it.
///
/// Move-only; a moved-from object is empty (gl_handle() is 0) and must not be used again. The window's GL
/// context must be current for every operation, see ABuffer for the rules.
class RIS_API GlesBufferObject {
public:
    /// Creates the GL buffer object and uploads `data` (null for uninitialised storage of `byte_size` bytes).
    /// The arguments must already be validated (see buffer_validation.hpp). `usage` must contain
    /// BufferUsageFlags::CopyDst for a buffer that is written later; it selects GL_DYNAMIC_DRAW, otherwise
    /// GL_STATIC_DRAW is used.
    /// @param type  selects the binding target (array, element array or uniform).
    /// @param label sets the GL object label (`glObjectLabel`) where `GL_KHR_debug` is available.
    /// @throws Error(NotInitialized) without a window, Error(Graphics) if GL fails.
    GlesBufferObject(BufferType type, BufferUsageFlags usage, const void* data, size_t byte_size, std::string label);

    /// The debug label, empty if none.
    const std::string& label() const noexcept { return label_; }
    /// Size of the GL storage in bytes.
    size_t byte_size() const noexcept { return buffer_.size(); }
    /// The usage flags stored at creation.
    BufferUsageFlags usage() const noexcept { return usage_; }
    /// The OpenGL buffer name, for interop with native code. 0 for a moved-from object.
    uint32_t gl_handle() const noexcept { return buffer_.gl_handle(); }
    /// The underlying low level buffer, e.g. for VertexArrayDesc::index_buffer. Owned by this object and valid
    /// only while it lives.
    const Buffer& buffer() const noexcept { return buffer_; }

    /// Writes `size` bytes at `byte_offset` (`glBufferSubData`).
    /// @param what name of the calling function for error messages.
    /// @throws Error(InvalidArgument) if the usage lacks CopyDst or the range / pointer is invalid (nothing
    ///         is written), Error(NotInitialized) without a window, Error(Graphics) if GL fails.
    void write(size_t byte_offset, const void* data, size_t size, const char* what);

    GlesBufferObject(const GlesBufferObject&) = delete;
    GlesBufferObject& operator=(const GlesBufferObject&) = delete;
    GlesBufferObject(GlesBufferObject&&) noexcept = default;
    GlesBufferObject& operator=(GlesBufferObject&&) noexcept = default;

private:
    Buffer buffer_;
    BufferUsageFlags usage_;
    std::string label_;
};

} // namespace ris::gles
