#include "ris/gles/buffers/gles_index_buffer.hpp"

#include <utility>
#include <vector>

#include "ris/rendering/buffers/buffer_validation.hpp"

namespace ris::gles {

GlesIndexBuffer::GlesIndexBuffer(const void* data, size_t byte_size, IndexBufferType type, std::string label)
    : object_(BufferType::Index, buffer_validation::resolve_index_buffer(byte_size), data, byte_size,
              std::move(label)),
      type_(type) {}

GlesIndexBuffer::GlesIndexBuffer(std::span<const uint16_t> indices, std::string label)
    : GlesIndexBuffer(indices.data(), indices.size_bytes(), IndexBufferType::UInt16, std::move(label)) {}

GlesIndexBuffer::GlesIndexBuffer(std::span<const uint32_t> indices, std::string label)
    : GlesIndexBuffer(
          [&]() -> GlesIndexBuffer {
              // Narrow to 16 bit when all indices fit, like the ris-framework-ts index buffer does.
              if (buffer_validation::select_index_type(indices) == IndexBufferType::UInt16 && !indices.empty()) {
                  const std::vector<uint16_t> narrow = buffer_validation::narrow_indices(indices);
                  return GlesIndexBuffer(std::span<const uint16_t>(narrow), std::move(label));
              }
              return GlesIndexBuffer(indices.data(), indices.size_bytes(), IndexBufferType::UInt32, std::move(label));
          }()) {}

GlesIndexBuffer GlesIndexBuffer::create_uint32(std::span<const uint32_t> indices, std::string label) {
    return GlesIndexBuffer(indices.data(), indices.size_bytes(), IndexBufferType::UInt32, std::move(label));
}

} // namespace ris::gles
