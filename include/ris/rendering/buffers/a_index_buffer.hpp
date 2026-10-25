#pragma once

#include <cstdint>

#include "ris/rendering/buffers/a_buffer.hpp"
#include "ris/rendering/buffers/index_buffer_type.hpp"

namespace ris {

/// An index buffer, like `IIndexBuffer` of ris-framework-ts. Abstract and graphics API neutral; create one
/// with ABufferFactory::create_index_buffer. The data is immutable after creation. See ABuffer for the
/// shared rules.
class RIS_API AIndexBuffer : public ABuffer {
public:
    ~AIndexBuffer() override = default;

    /// The stored element type (16 or 32 bit).
    virtual IndexBufferType type() const noexcept = 0;

    /// Size of one index in bytes, 2 or 4.
    uint32_t element_byte_size() const noexcept { return type() == IndexBufferType::UInt16 ? 2u : 4u; }
    /// Number of indices: byte_size() / element_byte_size().
    uint32_t indices_count() const noexcept { return static_cast<uint32_t>(byte_size() / element_byte_size()); }

protected:
    AIndexBuffer() = default;
    AIndexBuffer(AIndexBuffer&&) noexcept = default;
    AIndexBuffer& operator=(AIndexBuffer&&) noexcept = default;
};

} // namespace ris
