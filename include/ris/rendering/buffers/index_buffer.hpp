#pragma once

#include <cstdint>
#include <span>
#include <string>

#include "ris/graphics.hpp"
#include "ris/rendering/buffers/buffer.hpp"
#include "ris/rendering/buffers/index_buffer_type.hpp"

namespace ris {

/// An index buffer, like `IIndexBuffer` of ris-framework-ts. The data is immutable after creation.
class RIS_API IndexBuffer : public BufferBase {
public:
    /// Creates the buffer from 32 bit indices. Like ris-framework-ts it stores 16 bit indices when every
    /// index is <= 65535 and 32 bit indices otherwise, see type().
    /// @param indices the indices (copied), must not be empty.
    /// @param label   optional debug label.
    /// @throws Error(InvalidArgument) if empty, Error(NotInitialized) without a window,
    ///         Error(Graphics) if GL fails.
    explicit IndexBuffer(std::span<const uint32_t> indices, std::string label = {});
    /// Creates the buffer from 16 bit indices, stored as UInt16. Same errors as above.
    explicit IndexBuffer(std::span<const uint16_t> indices, std::string label = {});

    /// Creates the buffer from 32 bit indices and always stores them as UInt32.
    static IndexBuffer create_uint32(std::span<const uint32_t> indices, std::string label = {});

    /// The stored element type.
    IndexBufferType type() const noexcept { return type_; }
    /// The element type as the low level IndexType, for VertexArrayDesc::index_type.
    IndexType index_type() const noexcept {
        return type_ == IndexBufferType::UInt16 ? IndexType::UInt16 : IndexType::UInt32;
    }
    /// Size of one index in bytes, 2 or 4.
    uint32_t element_byte_size() const noexcept { return type_ == IndexBufferType::UInt16 ? 2u : 4u; }
    /// Number of indices.
    uint32_t indices_count() const noexcept { return static_cast<uint32_t>(byte_size() / element_byte_size()); }

private:
    IndexBuffer(const void* data, size_t byte_size, IndexBufferType type, std::string label);
    IndexBufferType type_ = IndexBufferType::UInt16;
};

} // namespace ris
