#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "ris/rendering/buffers/a_buffer.hpp"

namespace ris {

/// A vertex buffer of 32 bit floats, like `IVertexBuffer` of ris-framework-ts. Abstract and graphics API
/// neutral; create one with ABufferFactory::create_vertex_buffer. See ABuffer for the shared rules.
class RIS_API AVertexBuffer : public ABuffer {
public:
    ~AVertexBuffer() override = default;

    /// Size of one vertex in bytes, greater than zero.
    virtual uint32_t byte_stride() const noexcept = 0;

    /// Overwrites part of the buffer with `data`. The data is copied, the call does not keep the pointer.
    /// @param data        the floats to write.
    /// @param byte_offset where to start writing, in bytes.
    /// @throws Error(InvalidArgument) if the buffer lacks BufferUsageFlags::CopyDst or the range
    ///         [byte_offset, byte_offset + data bytes) is outside the buffer. Nothing is written in that case.
    virtual void update(std::span<const float> data, size_t byte_offset = 0) = 0;

    /// Number of vertices: byte_size() / byte_stride().
    uint32_t vertex_count() const noexcept {
        const uint32_t stride = byte_stride();
        return stride == 0 ? 0u : static_cast<uint32_t>(byte_size() / stride);
    }

protected:
    AVertexBuffer() = default;
    AVertexBuffer(AVertexBuffer&&) noexcept = default;
    AVertexBuffer& operator=(AVertexBuffer&&) noexcept = default;
};

} // namespace ris
