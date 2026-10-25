#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "ris/core.hpp"
#include "ris/graphics.hpp"
#include "ris/rendering/buffers/buffer_usage.hpp"

// Common rules of the TS-style typed GPU buffers (ris-framework-ts `IBuffer` and friends):
//  - a ris::Window must exist (its OpenGL ES context is current) while a buffer is created, updated or
//    destroyed, and everything must run on the window's thread;
//  - buffers are move-only and own their GL buffer object, which is deleted on destruction;
//  - failures throw ris::Error (argument problems are thrown before any GL call is made, so they do not
//    need a window).
// There is no C API for these types, use ris_buffer_* of ris.h from C.

namespace ris {

/// Common base of VertexBuffer, IndexBuffer and UniformBuffer, like `IBuffer` of ris-framework-ts.
/// Not constructible by users. Owns one GL buffer object, move-only; a moved-from buffer is empty and
/// must not be used again. Requires the window's GL context for every operation (see the rules in buffers.hpp).
class RIS_API BufferBase {
public:
    /// The debug label given at creation, empty if none. The label is kept on the CPU only.
    const std::string& label() const noexcept { return label_; }
    /// Size of the GPU storage in bytes. Uniform buffers report their padded size.
    size_t byte_size() const noexcept { return buffer_.size(); }
    /// The usage flags: the flags given at creation plus the flag of the buffer kind (e.g. Index).
    BufferUsageFlags usage() const noexcept { return usage_; }
    /// The OpenGL buffer name, for interop with native code. 0 for a moved-from buffer.
    uint32_t gl_handle() const noexcept { return buffer_.gl_handle(); }
    /// The underlying low level buffer, e.g. for VertexArrayDesc::index_buffer. Owned by this object and
    /// valid only while it lives.
    const Buffer& buffer() const noexcept { return buffer_; }

    BufferBase(const BufferBase&) = delete;
    BufferBase& operator=(const BufferBase&) = delete;

protected:
    BufferBase(BufferType type, BufferUsageFlags usage, const void* data, size_t byte_size, std::string label);
    BufferBase(BufferBase&&) noexcept = default;
    BufferBase& operator=(BufferBase&&) noexcept = default;
    ~BufferBase() = default;

    /// Writes `size` bytes at `byte_offset`. Throws Error(InvalidArgument) if outside the buffer, or
    /// if the buffer was not created with BufferUsageFlags::CopyDst.
    void write(size_t byte_offset, const void* data, size_t size, const char* what);

private:
    Buffer buffer_;
    BufferUsageFlags usage_;
    std::string label_;
};

} // namespace ris
