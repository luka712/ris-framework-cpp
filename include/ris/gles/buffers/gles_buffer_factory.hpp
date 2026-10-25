#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

#include "ris/rendering/buffers/a_buffer_factory.hpp"

namespace ris::gles {

/// The OpenGL ES implementation of ABufferFactory, like `WebGlBuffersFactory` of ris-framework-ts. It creates
/// GlesVertexBuffer, GlesIndexBuffer and GlesUniformBuffer objects and returns them as the abstract classes;
/// use gles::make_vertex_array_desc or a dynamic_cast to reach the native handles. Stateless, so it can be
/// created freely; see ABufferFactory for the arguments and errors of the functions.
class RIS_API GlesBufferFactory : public ABufferFactory {
public:
    GlesBufferFactory() = default;

    std::unique_ptr<AIndexBuffer> create_index_buffer(std::span<const uint32_t> indices,
                                                      std::string label = {}) const override;
    std::unique_ptr<AIndexBuffer> create_index_buffer(std::span<const uint16_t> indices,
                                                      std::string label = {}) const override;
    std::unique_ptr<AIndexBuffer> create_index_buffer_uint32(std::span<const uint32_t> indices,
                                                             std::string label = {}) const override;
    std::unique_ptr<AVertexBuffer> create_vertex_buffer(std::span<const float> data, uint32_t byte_stride,
                                                        BufferUsageFlags usage = BufferUsageFlags::Vertex,
                                                        std::string label = {}) const override;
    std::unique_ptr<AVertexBuffer> create_vertex_buffer_with_byte_size(
        size_t byte_size, uint32_t byte_stride,
        BufferUsageFlags usage = BufferUsageFlags::Vertex | BufferUsageFlags::CopyDst,
        std::string label = {}) const override;
    std::unique_ptr<AUniformBuffer> create_uniform_buffer(
        std::span<const float> data, BufferUsageFlags usage = BufferUsageFlags::Uniform | BufferUsageFlags::CopyDst,
        std::string label = {}) const override;
    std::unique_ptr<AUniformBuffer> create_uniform_buffer_with_byte_size(
        size_t byte_size, BufferUsageFlags usage = BufferUsageFlags::Uniform | BufferUsageFlags::CopyDst,
        std::string label = {}) const override;
};

} // namespace ris::gles
