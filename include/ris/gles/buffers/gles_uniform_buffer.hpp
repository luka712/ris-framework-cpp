#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "ris/gles/buffers/gles_buffer_object.hpp"
#include "ris/math.hpp"
#include "ris/rendering/buffers/a_uniform_buffer.hpp"

namespace ris::gles {

/// The OpenGL ES implementation of AUniformBuffer, like `WebGlUniformBuffer` of ris-framework-ts: a
/// `GL_UNIFORM_BUFFER`. Bind it to a uniform block binding point with bind_base() and
/// Shader::bind_uniform_block(). Lay the data out according to std140. Usually created through
/// GlesBufferFactory. Move-only RAII; see ABuffer and GlesBufferObject for the rules.
class RIS_API GlesUniformBuffer : public AUniformBuffer {
public:
    /// Creates the buffer and uploads `data`, zero padded to a multiple of 16 bytes (4 floats), like
    /// `AlignUtilities.alignArray(data, 4)` in ris-framework-ts.
    /// @param data   floats to upload (copied), must not be empty.
    /// @param usage  usage flags, add BufferUsageFlags::CopyDst if update() will be used. Flags of other
    ///               buffer kinds (Vertex, Index) are rejected.
    /// @param label  optional debug label.
    /// @throws std::invalid_argument for bad arguments, std::domain_error for Storage / MapRead,
    ///         std::logic_error without a window, std::runtime_error if GL fails.
    explicit GlesUniformBuffer(std::span<const float> data,
                               BufferUsageFlags usage = BufferUsageFlags::Uniform | BufferUsageFlags::CopyDst,
                               std::string label = {});

    /// Creates a zero filled buffer of `byte_size` bytes rounded up to a multiple of 16. Same errors as the
    /// constructor.
    static GlesUniformBuffer with_byte_size(size_t byte_size,
                                            BufferUsageFlags usage = BufferUsageFlags::Uniform | BufferUsageFlags::CopyDst,
                                            std::string label = {});

    GlesUniformBuffer(GlesUniformBuffer&&) noexcept = default;
    GlesUniformBuffer& operator=(GlesUniformBuffer&&) noexcept = default;

    const std::string& label() const noexcept override { return object_.label(); }
    size_t byte_size() const noexcept override { return object_.byte_size(); }
    BufferUsageFlags usage() const noexcept override { return object_.usage(); }

    /// See AUniformBuffer::update. The glm::mat4 and glm::vec4 overloads of the base class are available too.
    void update(std::span<const float> data, size_t byte_offset = 0) override;
    using AUniformBuffer::update;

    /// Binds the whole buffer to uniform block binding point `binding` (`glBindBufferBase`).
    void bind_base(uint32_t binding) const { object_.buffer().bind_base(binding); }

    /// The OpenGL buffer name, for interop with native code. 0 for a moved-from buffer.
    uint32_t gl_handle() const noexcept { return object_.gl_handle(); }
    /// The underlying low level buffer. Owned by this object and valid only while it lives.
    const Buffer& buffer() const noexcept { return object_.buffer(); }

private:
    GlesUniformBuffer(const void* data, size_t byte_size, BufferUsageFlags usage, std::string label);

    GlesBufferObject object_;
};

} // namespace ris::gles
