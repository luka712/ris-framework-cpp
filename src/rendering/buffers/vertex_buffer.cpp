#include "ris/rendering/buffers/vertex_buffer.hpp"

#include <utility>

#include "internal.hpp"
#include "rendering/buffers/buffers_internal.hpp"

using namespace ris::detail;

namespace ris {

namespace {

/// Validates the arguments of a vertex buffer and returns the usage to store.
BufferUsageFlags resolve_vertex(size_t byte_size, uint32_t byte_stride, BufferUsageFlags usage) {
    const BufferUsageFlags resolved = resolve_usage(usage, BufferUsageFlags::Vertex, "VertexBuffer");
    if (byte_stride == 0) {
        throw_error(Result::InvalidArgument, "VertexBuffer: byte_stride must be greater than zero");
    }
    if (byte_size == 0 || byte_size % byte_stride != 0) {
        throw_error(Result::InvalidArgument,
                    "VertexBuffer: data size %zu must be greater than zero and a multiple of the stride %u", byte_size,
                    byte_stride);
    }
    return resolved;
}

} // namespace

// ---- VertexBuffer ---------------------------------------------------------------------------

VertexBuffer::VertexBuffer(const void* data, size_t byte_size, uint32_t byte_stride, BufferUsageFlags usage,
                           std::string label)
    : BufferBase(BufferType::Vertex, resolve_vertex(byte_size, byte_stride, usage), data, byte_size, std::move(label)),
      byte_stride_(byte_stride) {}

VertexBuffer::VertexBuffer(std::span<const float> data, uint32_t byte_stride, BufferUsageFlags usage,
                           std::string label)
    : VertexBuffer(data.data(), data.size_bytes(), byte_stride, usage, std::move(label)) {}

VertexBuffer VertexBuffer::with_byte_size(size_t byte_size, uint32_t byte_stride, BufferUsageFlags usage,
                                          std::string label) {
    return VertexBuffer(nullptr, byte_size, byte_stride, usage, std::move(label));
}

void VertexBuffer::update(std::span<const float> data, size_t byte_offset) {
    write(byte_offset, data.data(), data.size_bytes(), "VertexBuffer::update");
}

} // namespace ris
