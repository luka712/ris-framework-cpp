#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "ris/gles/buffers/gles_buffer_object.hpp"
#include "ris/graphics.hpp"
#include "ris/rendering/buffers/a_vertex_buffer.hpp"

namespace ris::gles {

/// The OpenGL ES implementation of AVertexBuffer, like `WebGlVertexBuffer` of ris-framework-ts: a
/// `GL_ARRAY_BUFFER` of 32 bit floats. Usually created through GlesBufferFactory; the constructors are public
/// for code that wants the concrete type and the native handle. Move-only RAII; see ABuffer and
/// GlesBufferObject for the rules.
class RIS_API GlesVertexBuffer : public AVertexBuffer {
public:
    /// Creates the buffer and uploads `data`.
    /// @param data        floats to upload (copied). Must not be empty and its byte size must be a multiple
    ///                    of `byte_stride`.
    /// @param byte_stride size of one vertex in bytes, greater than zero.
    /// @param usage       usage flags. Add BufferUsageFlags::CopyDst if update() will be used. Flags of other
    ///                    buffer kinds (Index, Uniform) are rejected.
    /// @param label       optional debug label.
    /// @throws std::invalid_argument for bad arguments, std::domain_error for Storage / MapRead,
    ///         std::logic_error without a window, std::runtime_error if GL fails.
    GlesVertexBuffer(std::span<const float> data, uint32_t byte_stride,
                     BufferUsageFlags usage = BufferUsageFlags::Vertex, std::string label = {});

    /// Creates a buffer with uninitialised contents of `byte_size` bytes (a multiple of `byte_stride`).
    /// Fill it with update(), so pass BufferUsageFlags::CopyDst. Same errors as the constructor.
    static GlesVertexBuffer with_byte_size(size_t byte_size, uint32_t byte_stride,
                                           BufferUsageFlags usage = BufferUsageFlags::Vertex | BufferUsageFlags::CopyDst,
                                           std::string label = {});

    GlesVertexBuffer(GlesVertexBuffer&&) noexcept = default;
    GlesVertexBuffer& operator=(GlesVertexBuffer&&) noexcept = default;

    const std::string& label() const noexcept override { return object_.label(); }
    size_t byte_size() const noexcept override { return object_.byte_size(); }
    BufferUsageFlags usage() const noexcept override { return object_.usage(); }
    uint32_t byte_stride() const noexcept override { return byte_stride_; }

    /// See AVertexBuffer::update.
    void update(std::span<const float> data, size_t byte_offset = 0) override;

    /// The OpenGL buffer name, for interop with native code. 0 for a moved-from buffer.
    uint32_t gl_handle() const noexcept { return object_.gl_handle(); }
    /// The underlying low level buffer. Owned by this object and valid only while it lives.
    const Buffer& buffer() const noexcept { return object_.buffer(); }
    /// The binding to put into VertexArrayDesc::vertex_buffers. Refers to this buffer, which must outlive the
    /// VertexArray.
    VertexBufferBinding binding() const noexcept { return {&buffer(), byte_stride_}; }

private:
    GlesVertexBuffer(const void* data, size_t byte_size, uint32_t byte_stride, BufferUsageFlags usage,
                     std::string label);

    GlesBufferObject object_;
    uint32_t byte_stride_ = 0;
};

} // namespace ris::gles
