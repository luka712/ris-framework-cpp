#include "ris/gles/buffers/gles_buffer_factory.hpp"

#include <memory>
#include <utility>

#include "ris/gles/buffers/gles_index_buffer.hpp"
#include "ris/gles/buffers/gles_uniform_buffer.hpp"
#include "ris/gles/buffers/gles_vertex_buffer.hpp"

namespace ris::gles {

std::unique_ptr<AIndexBuffer> GlesBufferFactory::create_index_buffer(std::span<const uint32_t> indices,
                                                                     std::string label) const {
    return std::make_unique<GlesIndexBuffer>(indices, std::move(label));
}

std::unique_ptr<AIndexBuffer> GlesBufferFactory::create_index_buffer(std::span<const uint16_t> indices,
                                                                     std::string label) const {
    return std::make_unique<GlesIndexBuffer>(indices, std::move(label));
}

std::unique_ptr<AIndexBuffer> GlesBufferFactory::create_index_buffer_uint32(std::span<const uint32_t> indices,
                                                                            std::string label) const {
    return std::make_unique<GlesIndexBuffer>(GlesIndexBuffer::create_uint32(indices, std::move(label)));
}

std::unique_ptr<AVertexBuffer> GlesBufferFactory::create_vertex_buffer(std::span<const float> data,
                                                                       uint32_t byte_stride, BufferUsageFlags usage,
                                                                       std::string label) const {
    return std::make_unique<GlesVertexBuffer>(data, byte_stride, usage, std::move(label));
}

std::unique_ptr<AVertexBuffer> GlesBufferFactory::create_vertex_buffer_with_byte_size(
    size_t byte_size, uint32_t byte_stride, BufferUsageFlags usage, std::string label) const {
    return std::make_unique<GlesVertexBuffer>(
        GlesVertexBuffer::with_byte_size(byte_size, byte_stride, usage, std::move(label)));
}

std::unique_ptr<AUniformBuffer> GlesBufferFactory::create_uniform_buffer(std::span<const float> data,
                                                                         BufferUsageFlags usage,
                                                                         std::string label) const {
    return std::make_unique<GlesUniformBuffer>(data, usage, std::move(label));
}

std::unique_ptr<AUniformBuffer> GlesBufferFactory::create_uniform_buffer_with_byte_size(
    size_t byte_size, BufferUsageFlags usage, std::string label) const {
    return std::make_unique<GlesUniformBuffer>(
        GlesUniformBuffer::with_byte_size(byte_size, usage, std::move(label)));
}

} // namespace ris::gles
