#pragma once

#include <cstdint>
#include <span>
#include <string>

#include "ris/gles/buffers/gles_buffer_object.hpp"
#include "ris/graphics.hpp"
#include "ris/rendering/buffers/a_index_buffer.hpp"

namespace ris::gles {

/// The OpenGL ES implementation of AIndexBuffer, like `WebGlIndexBuffer` of ris-framework-ts: a
/// `GL_ELEMENT_ARRAY_BUFFER`. The data is immutable after creation. Usually created through
/// GlesBufferFactory. Move-only RAII; see ABuffer and GlesBufferObject for the rules.
class RIS_API GlesIndexBuffer : public AIndexBuffer {
public:
    /// Creates the buffer from 32 bit indices. Like ris-framework-ts it stores 16 bit indices when every index
    /// is <= 65535 and 32 bit indices otherwise, see type().
    /// @param indices the indices (copied), must not be empty.
    /// @param label   optional debug label.
    /// @throws Error(InvalidArgument) if empty, Error(NotInitialized) without a window,
    ///         Error(Graphics) if GL fails.
    explicit GlesIndexBuffer(std::span<const uint32_t> indices, std::string label = {});
    /// Creates the buffer from 16 bit indices, stored as UInt16. Same errors as above.
    explicit GlesIndexBuffer(std::span<const uint16_t> indices, std::string label = {});

    /// Creates the buffer from 32 bit indices and always stores them as UInt32.
    static GlesIndexBuffer create_uint32(std::span<const uint32_t> indices, std::string label = {});

    GlesIndexBuffer(GlesIndexBuffer&&) noexcept = default;
    GlesIndexBuffer& operator=(GlesIndexBuffer&&) noexcept = default;

    const std::string& label() const noexcept override { return object_.label(); }
    size_t byte_size() const noexcept override { return object_.byte_size(); }
    BufferUsageFlags usage() const noexcept override { return object_.usage(); }
    IndexBufferType type() const noexcept override { return type_; }

    /// The element type as the low level IndexType, for VertexArrayDesc::index_type.
    IndexType index_type() const noexcept {
        return type_ == IndexBufferType::UInt16 ? IndexType::UInt16 : IndexType::UInt32;
    }
    /// The OpenGL buffer name, for interop with native code. 0 for a moved-from buffer.
    uint32_t gl_handle() const noexcept { return object_.gl_handle(); }
    /// The underlying low level buffer, e.g. for VertexArrayDesc::index_buffer. Owned by this object and valid
    /// only while it lives.
    const Buffer& buffer() const noexcept { return object_.buffer(); }

private:
    GlesIndexBuffer(const void* data, size_t byte_size, IndexBufferType type, std::string label);

    GlesBufferObject object_;
    IndexBufferType type_ = IndexBufferType::UInt16;
};

} // namespace ris::gles
