#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "ris/graphics.hpp"
#include "ris/rendering/buffers/buffer.hpp"
#include "ris/rendering/buffers/buffer_usage.hpp"

namespace ris {

/// A vertex buffer of 32 bit floats, like `IVertexBuffer` of ris-framework-ts.
class RIS_API VertexBuffer : public BufferBase {
public:
    /// Creates the buffer and uploads `data`.
    /// @param data     floats to upload (copied). Must not be empty and its byte size must be a multiple
    ///                 of `byte_stride`.
    /// @param byte_stride size of one vertex in bytes, greater than zero.
    /// @param usage    usage flags. Add BufferUsageFlags::CopyDst if update() will be used. Flags of other
    ///                 buffer kinds (Index, Uniform) are rejected.
    /// @param label    optional debug label.
    /// @throws Error(InvalidArgument) for bad arguments, Error(Unsupported) for Storage / MapRead,
    ///         Error(NotInitialized) without a window, Error(Graphics) if GL fails.
    VertexBuffer(std::span<const float> data, uint32_t byte_stride,
                 BufferUsageFlags usage = BufferUsageFlags::Vertex, std::string label = {});

    /// Creates a buffer with uninitialised contents of `byte_size` bytes (a multiple of `byte_stride`).
    /// Fill it with update(), so pass BufferUsageFlags::CopyDst. Same errors as the constructor.
    static VertexBuffer with_byte_size(size_t byte_size, uint32_t byte_stride,
                                       BufferUsageFlags usage = BufferUsageFlags::Vertex | BufferUsageFlags::CopyDst,
                                       std::string label = {});

    /// Number of vertices: byte_size() / byte_stride().
    uint32_t vertex_count() const noexcept { return static_cast<uint32_t>(byte_size() / byte_stride_); }
    /// Size of one vertex in bytes.
    uint32_t byte_stride() const noexcept { return byte_stride_; }

    /// Overwrites part of the buffer with `data` (copied, the call does not keep the pointer).
    /// @param byte_offset where to start writing, in bytes.
    /// @throws Error(InvalidArgument) if the buffer lacks CopyDst or [byte_offset, byte_offset + data
    ///         bytes) is outside the buffer. Nothing is written in that case.
    void update(std::span<const float> data, size_t byte_offset = 0);

    /// The binding to put into VertexArrayDesc::vertex_buffers. Refers to this buffer, which must outlive
    /// the VertexArray.
    VertexBufferBinding binding() const noexcept { return {&buffer(), byte_stride_}; }

private:
    VertexBuffer(const void* data, size_t byte_size, uint32_t byte_stride, BufferUsageFlags usage, std::string label);
    uint32_t byte_stride_ = 0;
};

} // namespace ris
