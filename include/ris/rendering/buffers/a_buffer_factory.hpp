#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

#include "ris/core.hpp"
#include "ris/rendering/buffers/a_index_buffer.hpp"
#include "ris/rendering/buffers/a_uniform_buffer.hpp"
#include "ris/rendering/buffers/a_vertex_buffer.hpp"
#include "ris/rendering/buffers/buffer_usage.hpp"

namespace ris {

/// Creates buffers, like `IBufferFactory` of ris-framework-ts. Abstract and graphics API neutral: every
/// rendering backend provides one implementation (GlesBufferFactory for OpenGL ES). Get the factory of the
/// default backend with create_default_buffer_factory() (ris/rendering/default_buffer_factory.hpp).
///
/// The factory is stateless and cheap. The buffers it creates do not refer to it and may outlive it. The
/// same rules and errors as in ABuffer apply to every function; the arguments are validated with
/// the helpers of buffer_validation.hpp, so all backends report the same errors for the same bad arguments.
class RIS_API ABufferFactory {
public:
    virtual ~ABufferFactory() = default;

    /// Creates an index buffer from 32 bit indices. Like ris-framework-ts it stores 16 bit indices when every
    /// index is <= 65535 and 32 bit indices otherwise, see AIndexBuffer::type().
    /// @param indices the indices (copied), must not be empty.
    /// @param label   optional debug label.
    /// @throws std::invalid_argument if empty, std::logic_error without a device, std::runtime_error if
    ///         the backend fails.
    virtual std::unique_ptr<AIndexBuffer> create_index_buffer(std::span<const uint32_t> indices,
                                                              std::string label = {}) const = 0;
    /// Creates an index buffer from 16 bit indices, stored as UInt16. Same errors as above.
    virtual std::unique_ptr<AIndexBuffer> create_index_buffer(std::span<const uint16_t> indices,
                                                              std::string label = {}) const = 0;
    /// Creates an index buffer from 32 bit indices and always stores them as UInt32.
    virtual std::unique_ptr<AIndexBuffer> create_index_buffer_uint32(std::span<const uint32_t> indices,
                                                                     std::string label = {}) const = 0;

    /// Creates a vertex buffer and uploads `data`.
    /// @param data        floats to upload (copied). Must not be empty and its byte size must be a multiple
    ///                    of `byte_stride`.
    /// @param byte_stride size of one vertex in bytes, greater than zero.
    /// @param usage       usage flags. Add BufferUsageFlags::CopyDst if AVertexBuffer::update will be used.
    ///                    Flags of other buffer kinds (Index, Uniform) are rejected.
    /// @param label       optional debug label.
    /// @throws std::invalid_argument for bad arguments, std::domain_error for usage flags the backend
    ///         cannot honour (Storage / MapRead on OpenGL ES 3.0), std::logic_error without a device,
    ///         std::runtime_error if the backend fails.
    virtual std::unique_ptr<AVertexBuffer> create_vertex_buffer(std::span<const float> data, uint32_t byte_stride,
                                                                BufferUsageFlags usage = BufferUsageFlags::Vertex,
                                                                std::string label = {}) const = 0;
    /// Creates a vertex buffer with uninitialised contents of `byte_size` bytes (a multiple of `byte_stride`).
    /// Fill it with AVertexBuffer::update, so keep CopyDst in `usage`. Same errors as above.
    virtual std::unique_ptr<AVertexBuffer> create_vertex_buffer_with_byte_size(
        size_t byte_size, uint32_t byte_stride,
        BufferUsageFlags usage = BufferUsageFlags::Vertex | BufferUsageFlags::CopyDst,
        std::string label = {}) const = 0;

    /// Creates a uniform buffer and uploads `data`, zero padded to a multiple of 16 bytes (4 floats), like
    /// `AlignUtilities.alignArray(data, 4)` in ris-framework-ts.
    /// @param data   floats to upload (copied), must not be empty.
    /// @param usage  usage flags, add BufferUsageFlags::CopyDst if AUniformBuffer::update will be used. Flags of
    ///               other buffer kinds (Vertex, Index) are rejected.
    /// @param label  optional debug label.
    /// @throws std::invalid_argument for bad arguments, std::domain_error for usage flags the backend
    ///         cannot honour, std::logic_error without a device, std::runtime_error if the backend fails.
    virtual std::unique_ptr<AUniformBuffer> create_uniform_buffer(
        std::span<const float> data, BufferUsageFlags usage = BufferUsageFlags::Uniform | BufferUsageFlags::CopyDst,
        std::string label = {}) const = 0;
    /// Creates a zero filled uniform buffer of `byte_size` bytes rounded up to a multiple of 16. Same errors as
    /// above.
    virtual std::unique_ptr<AUniformBuffer> create_uniform_buffer_with_byte_size(
        size_t byte_size, BufferUsageFlags usage = BufferUsageFlags::Uniform | BufferUsageFlags::CopyDst,
        std::string label = {}) const = 0;

    ABufferFactory(const ABufferFactory&) = delete;
    ABufferFactory& operator=(const ABufferFactory&) = delete;

protected:
    ABufferFactory() = default;
};

} // namespace ris
