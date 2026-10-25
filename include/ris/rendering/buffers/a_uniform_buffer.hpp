#pragma once

#include <cstddef>
#include <span>

#include "ris/math.hpp"
#include "ris/rendering/buffers/a_buffer.hpp"

namespace ris {

/// A uniform buffer, like `IUniformBuffer` of ris-framework-ts. Abstract and graphics API neutral; create one
/// with ABufferFactory::create_uniform_buffer. Lay the data out according to std140. The storage is padded
/// to a multiple of 16 bytes (see buffer_validation::uniform_buffer_alignment). Binding the buffer to a
/// shader is backend specific and done through the concrete class (e.g. GlesUniformBuffer::bind_base).
/// See ABuffer for the shared rules.
class RIS_API AUniformBuffer : public ABuffer {
public:
    ~AUniformBuffer() override = default;

    /// Overwrites part of the buffer with `data`. The data is copied.
    /// @param data        the floats to write.
    /// @param byte_offset where to start writing, in bytes. The range must be inside the (padded) buffer.
    /// @throws Error(InvalidArgument) if the buffer lacks BufferUsageFlags::CopyDst or the range is outside
    ///         the buffer. Nothing is written in that case.
    virtual void update(std::span<const float> data, size_t byte_offset = 0) = 0;

    /// Writes a column-major matrix (64 bytes) at `byte_offset`. Same rules as the span overload.
    void update(const Mat4& matrix, size_t byte_offset = 0) {
        update(std::span<const float>(glm::value_ptr(matrix), 16), byte_offset);
    }
    /// Writes a vector (16 bytes) at `byte_offset`. Same rules as the span overload.
    void update(const Vec4& vector, size_t byte_offset = 0) {
        update(std::span<const float>(glm::value_ptr(vector), 4), byte_offset);
    }

protected:
    AUniformBuffer() = default;
    AUniformBuffer(AUniformBuffer&&) noexcept = default;
    AUniformBuffer& operator=(AUniformBuffer&&) noexcept = default;
};

} // namespace ris
