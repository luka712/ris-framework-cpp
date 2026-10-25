#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>

#include "ris/core.hpp"
#include "ris/rendering/buffers/buffer_usage.hpp"
#include "ris/rendering/buffers/index_buffer.hpp"
#include "ris/rendering/buffers/uniform_buffer.hpp"
#include "ris/rendering/buffers/vertex_buffer.hpp"

namespace ris {

/// Creates buffers, like `IBufferFactory` of ris-framework-ts. Stateless; the buffers are returned in
/// `std::unique_ptr`s because ris-framework-ts hands out interfaces. The same rules and errors as the
/// buffer constructors apply.
class RIS_API BufferFactory {
public:
    /// See IndexBuffer::IndexBuffer(std::span<const uint32_t>, std::string).
    std::unique_ptr<IndexBuffer> create_index_buffer(std::span<const uint32_t> indices, std::string label = {}) const;
    /// See VertexBuffer::VertexBuffer.
    std::unique_ptr<VertexBuffer> create_vertex_buffer(std::span<const float> data, uint32_t byte_stride,
                                                       BufferUsageFlags usage = BufferUsageFlags::Vertex,
                                                       std::string label = {}) const;
    /// See UniformBuffer::UniformBuffer.
    std::unique_ptr<UniformBuffer> create_uniform_buffer(
        std::span<const float> data,
        BufferUsageFlags usage = BufferUsageFlags::Uniform | BufferUsageFlags::CopyDst, std::string label = {}) const;
};

} // namespace ris
