#include "ris/rendering/buffers/uniform_buffer.hpp"

#include <algorithm>
#include <utility>
#include <vector>

#include "internal.hpp"
#include "rendering/buffers/buffers_internal.hpp"

using namespace ris::detail;

namespace ris {

namespace {

size_t align_up_16(size_t bytes) { return (bytes + 15) & ~size_t{15}; }

} // namespace

// ---- UniformBuffer --------------------------------------------------------------------------

UniformBuffer::UniformBuffer(const void* data, size_t byte_size, BufferUsageFlags usage, std::string label)
    : BufferBase(BufferType::Uniform, resolve_usage(usage, BufferUsageFlags::Uniform, "UniformBuffer"), data,
                 byte_size, std::move(label)) {}

UniformBuffer::UniformBuffer(std::span<const float> data, BufferUsageFlags usage, std::string label)
    : UniformBuffer(
          [&]() -> UniformBuffer {
              if (data.empty()) throw_error(Result::InvalidArgument, "UniformBuffer: data must not be empty");
              std::vector<float> padded(align_up_16(data.size_bytes()) / sizeof(float), 0.0f);
              std::copy(data.begin(), data.end(), padded.begin());
              return UniformBuffer(padded.data(), padded.size() * sizeof(float), usage, std::move(label));
          }()) {}

UniformBuffer UniformBuffer::with_byte_size(size_t byte_size, BufferUsageFlags usage, std::string label) {
    if (byte_size == 0) throw_error(Result::InvalidArgument, "UniformBuffer: byte_size must be greater than zero");
    const std::vector<float> zeros(align_up_16(byte_size) / sizeof(float), 0.0f);
    return UniformBuffer(zeros.data(), zeros.size() * sizeof(float), usage, std::move(label));
}

void UniformBuffer::update(std::span<const float> data, size_t byte_offset) {
    write(byte_offset, data.data(), data.size_bytes(), "UniformBuffer::update");
}

void UniformBuffer::update(const Mat4& matrix, size_t byte_offset) {
    write(byte_offset, glm::value_ptr(matrix), sizeof(float) * 16, "UniformBuffer::update");
}

void UniformBuffer::update(const Vec4& vector, size_t byte_offset) {
    write(byte_offset, glm::value_ptr(vector), sizeof(float) * 4, "UniformBuffer::update");
}

} // namespace ris
