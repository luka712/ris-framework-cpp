#include "ris/rendering/buffers/buffer_factory.hpp"

#include <memory>
#include <utility>

namespace ris {

// ---- BufferFactory --------------------------------------------------------------------------

std::unique_ptr<IndexBuffer> BufferFactory::create_index_buffer(std::span<const uint32_t> indices,
                                                                std::string label) const {
    return std::make_unique<IndexBuffer>(indices, std::move(label));
}

std::unique_ptr<VertexBuffer> BufferFactory::create_vertex_buffer(std::span<const float> data, uint32_t byte_stride,
                                                                  BufferUsageFlags usage, std::string label) const {
    return std::make_unique<VertexBuffer>(data, byte_stride, usage, std::move(label));
}

std::unique_ptr<UniformBuffer> BufferFactory::create_uniform_buffer(std::span<const float> data,
                                                                    BufferUsageFlags usage, std::string label) const {
    return std::make_unique<UniformBuffer>(data, usage, std::move(label));
}

} // namespace ris
