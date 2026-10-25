#include "ris/rendering/buffers/index_buffer.hpp"

#include <algorithm>
#include <utility>
#include <vector>

#include "internal.hpp"

using namespace ris::detail;

namespace ris {

namespace {
BufferUsageFlags resolve_index(size_t byte_size) {
    if (byte_size == 0) throw_error(Result::InvalidArgument, "IndexBuffer: indices must not be empty");
    return BufferUsageFlags::Index;
}
} // namespace

IndexBuffer::IndexBuffer(const void* data, size_t byte_size, IndexBufferType type, std::string label)
    : BufferBase(BufferType::Index, resolve_index(byte_size), data, byte_size, std::move(label)), type_(type) {}

IndexBuffer::IndexBuffer(std::span<const uint16_t> indices, std::string label)
    : IndexBuffer(indices.data(), indices.size_bytes(), IndexBufferType::UInt16, std::move(label)) {}

namespace {
/// Narrows 32 bit indices to 16 bit when all fit, like the ris-framework-ts index buffer does.
std::vector<uint16_t> narrow_indices(std::span<const uint32_t> indices) {
    std::vector<uint16_t> out;
    const bool fits = std::all_of(indices.begin(), indices.end(), [](uint32_t i) { return i <= 0xFFFFu; });
    if (fits) out.assign(indices.begin(), indices.end());
    return out;
}
} // namespace

IndexBuffer::IndexBuffer(std::span<const uint32_t> indices, std::string label)
    : IndexBuffer(
          [&]() -> IndexBuffer {
              const std::vector<uint16_t> narrow = narrow_indices(indices);
              if (!narrow.empty()) return IndexBuffer(std::span<const uint16_t>(narrow), std::move(label));
              return IndexBuffer(indices.data(), indices.size_bytes(), IndexBufferType::UInt32, std::move(label));
          }()) {}

IndexBuffer IndexBuffer::create_uint32(std::span<const uint32_t> indices, std::string label) {
    return IndexBuffer(indices.data(), indices.size_bytes(), IndexBufferType::UInt32, std::move(label));
}

} // namespace ris
