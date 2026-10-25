#include "ris/gles/buffers/gles_uniform_buffer.hpp"

#include <utility>
#include <vector>

#include "ris/rendering/buffers/buffer_validation.hpp"

namespace ris::gles {

namespace {
/// OpenGL ES 3.0 has no storage buffers and no CPU read back.
constexpr BufferUsageFlags unsupported_usage = BufferUsageFlags::Storage | BufferUsageFlags::MapRead;
constexpr const char* api_name = "OpenGL ES 3.0";
} // namespace

GlesUniformBuffer::GlesUniformBuffer(const void* data, size_t byte_size, BufferUsageFlags usage, std::string label)
    : object_(BufferType::Uniform,
              buffer_validation::resolve_usage(usage, BufferUsageFlags::Uniform, "UniformBuffer", unsupported_usage,
                                               api_name),
              data, byte_size, std::move(label)) {}

GlesUniformBuffer::GlesUniformBuffer(std::span<const float> data, BufferUsageFlags usage, std::string label)
    : GlesUniformBuffer(
          [&]() -> GlesUniformBuffer {
              const std::vector<float> padded = buffer_validation::pad_uniform_data(data);
              return GlesUniformBuffer(padded.data(), padded.size() * sizeof(float), usage, std::move(label));
          }()) {}

GlesUniformBuffer GlesUniformBuffer::with_byte_size(size_t byte_size, BufferUsageFlags usage, std::string label) {
    const std::vector<float> zeros(buffer_validation::padded_uniform_byte_size(byte_size) / sizeof(float), 0.0f);
    return GlesUniformBuffer(zeros.data(), zeros.size() * sizeof(float), usage, std::move(label));
}

void GlesUniformBuffer::update(std::span<const float> data, size_t byte_offset) {
    object_.write(byte_offset, data.data(), data.size_bytes(), "UniformBuffer::update");
}

} // namespace ris::gles
