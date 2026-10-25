#include "ris/gles/buffers/gles_vertex_buffer.hpp"

#include <utility>

#include "ris/rendering/buffers/buffer_validation.hpp"

namespace ris::gles {

namespace {
/// OpenGL ES 3.0 has no storage buffers and no CPU read back.
constexpr BufferUsageFlags unsupported_usage = BufferUsageFlags::Storage | BufferUsageFlags::MapRead;
constexpr const char* api_name = "OpenGL ES 3.0";
} // namespace

GlesVertexBuffer::GlesVertexBuffer(const void* data, size_t byte_size, uint32_t byte_stride, BufferUsageFlags usage,
                                   std::string label)
    : object_(BufferType::Vertex,
              buffer_validation::resolve_vertex_buffer(byte_size, byte_stride, usage, unsupported_usage, api_name),
              data, byte_size, std::move(label)),
      byte_stride_(byte_stride) {}

GlesVertexBuffer::GlesVertexBuffer(std::span<const float> data, uint32_t byte_stride, BufferUsageFlags usage,
                                   std::string label)
    : GlesVertexBuffer(data.data(), data.size_bytes(), byte_stride, usage, std::move(label)) {}

GlesVertexBuffer GlesVertexBuffer::with_byte_size(size_t byte_size, uint32_t byte_stride, BufferUsageFlags usage,
                                                  std::string label) {
    return GlesVertexBuffer(nullptr, byte_size, byte_stride, usage, std::move(label));
}

void GlesVertexBuffer::update(std::span<const float> data, size_t byte_offset) {
    object_.write(byte_offset, data.data(), data.size_bytes(), "VertexBuffer::update");
}

} // namespace ris::gles
