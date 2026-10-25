#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "ris/math.hpp"
#include "ris/rendering/buffers/buffer.hpp"
#include "ris/rendering/buffers/buffer_usage.hpp"

namespace ris {

/// A uniform buffer, like `IUniformBuffer` of ris-framework-ts. Bind it to a uniform block binding point
/// with bind_base() and Shader::bind_uniform_block(). Lay the data out according to std140.
class RIS_API UniformBuffer : public BufferBase {
public:
    /// Creates the buffer and uploads `data`, zero padded to a multiple of 16 bytes (4 floats), like
    /// `AlignUtilities.alignArray(data, 4)` in ris-framework-ts.
    /// @param data   floats to upload (copied), must not be empty.
    /// @param usage  usage flags, add BufferUsageFlags::CopyDst if update() will be used. Flags of other
    ///               buffer kinds (Vertex, Index) are rejected.
    /// @param label  optional debug label.
    /// @throws Error(InvalidArgument) for bad arguments, Error(Unsupported) for Storage / MapRead,
    ///         Error(NotInitialized) without a window, Error(Graphics) if GL fails.
    explicit UniformBuffer(std::span<const float> data,
                           BufferUsageFlags usage = BufferUsageFlags::Uniform | BufferUsageFlags::CopyDst,
                           std::string label = {});

    /// Creates a zero filled buffer of `byte_size` bytes rounded up to a multiple of 16. Same errors as
    /// the constructor.
    static UniformBuffer with_byte_size(size_t byte_size,
                                        BufferUsageFlags usage = BufferUsageFlags::Uniform | BufferUsageFlags::CopyDst,
                                        std::string label = {});

    /// Overwrites part of the buffer with `data` (copied).
    /// @param byte_offset where to start writing, in bytes. Must be inside the (padded) buffer.
    /// @throws Error(InvalidArgument) if the buffer lacks CopyDst or the range is outside the buffer.
    ///         Nothing is written in that case.
    void update(std::span<const float> data, size_t byte_offset = 0);
    /// Writes a column-major matrix (64 bytes).
    void update(const Mat4& matrix, size_t byte_offset = 0);
    /// Writes a vector (16 bytes).
    void update(const Vec4& vector, size_t byte_offset = 0);

    /// Binds the whole buffer to uniform block binding point `binding` (glBindBufferBase).
    void bind_base(uint32_t binding) const { buffer().bind_base(binding); }

private:
    UniformBuffer(const void* data, size_t byte_size, BufferUsageFlags usage, std::string label);
};

} // namespace ris
