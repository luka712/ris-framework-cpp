#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "ris/core.hpp"
#include "ris/graphics.hpp"
#include "ris/rendering/buffers/buffer_usage.hpp"

namespace ris::gles {

/// Creating and updating raw GL buffer objects, `WebGlBufferUtilities` of ris-framework-ts. The Gles buffers
/// (GlesVertexBuffer, GlesIndexBuffer, GlesUniformBuffer) and the low level ris::Buffer are built on these.
/// Reached through `GlesUtilities::buffer()`.
///
/// The functions return the plain GL buffer name, which you own and must delete with deleteBuffer().
/// They need the window's GL context and throw std::logic_error without one.
///
/// Unlike the TypeScript utilities, which bind `ELEMENT_ARRAY_BUFFER` and unbind it afterwards (which
/// detaches the index buffer of the current vertex array in WebGL, too), index buffers are bound to
/// `GL_COPY_WRITE_BUFFER` while they are filled, so the current vertex array is never touched. No target is
/// left bound. Debug labels are set (KHR_debug, if available) once the buffer object exists, i.e. after the
/// upload; the TypeScript version labels the object first.
class RIS_API GlesBufferUtilities {
public:
    /// Creates a buffer of any kind.
    /// @param type     the kind of buffer, selects the binding target (see converter::convert_buffer_target).
    /// @param data     initial contents, or null for uninitialised storage of `byteSize` bytes.
    /// @param byteSize size in bytes, greater than zero.
    /// @param usage    the `glBufferData` usage hint, see converter::convert_buffer_usage.
    /// @param label    optional debug label, see debug_label_utilities.
    /// @return the buffer name, never 0.
    /// @throws std::invalid_argument for a zero size, std::runtime_error if GL reports an error (the buffer is
    ///         deleted in that case) or no buffer name could be generated ("Failed to create vertex buffer.",
    ///         "Failed to create index buffer." or "Failed to create uniform buffer.", by `type`).
    uint32_t createBuffer(BufferType type, const void* data, size_t byteSize, uint32_t usage,
                        std::string_view label = {}) const;

    /// `createVertexBuffer` of ris-framework-ts: a `GL_ARRAY_BUFFER` with the given floats.
    /// @param data  vertex data, must not be empty.
    /// @param usage usage flags, only `CopyDst` matters (dynamic or static draw).
    /// @param label optional debug label.
    /// @throws std::invalid_argument for empty data, std::runtime_error ("Failed to create vertex buffer." if no name could be generated).
    uint32_t createVertexBuffer(std::span<const float> data, BufferUsageFlags usage, std::string_view label = {}) const;
    /// A `GL_ARRAY_BUFFER` of `byteSize` uninitialised bytes (the `number` overload of the TypeScript function).
    uint32_t createVertexBuffer(size_t byteSize, BufferUsageFlags usage, std::string_view label = {}) const;

    /// `createIndexBuffer` of ris-framework-ts: an element array buffer with 16 bit indices.
    /// @throws std::invalid_argument for empty data, std::runtime_error ("Failed to create index buffer." if no name could be generated).
    uint32_t createIndexBuffer(std::span<const uint16_t> indices, BufferUsageFlags usage,
                             std::string_view label = {}) const;
    /// An element array buffer with 32 bit indices.
    uint32_t createIndexBuffer(std::span<const uint32_t> indices, BufferUsageFlags usage,
                             std::string_view label = {}) const;
    /// An element array buffer of `byteSize` uninitialised bytes.
    uint32_t createIndexBuffer(size_t byteSize, BufferUsageFlags usage, std::string_view label = {}) const;

    /// `createUniformBuffer` of ris-framework-ts: a `GL_UNIFORM_BUFFER` with the given floats. The data is not
    /// padded here (GlesUniformBuffer does that).
    /// @throws std::invalid_argument for empty data, std::runtime_error ("Failed to create uniform buffer." if no name could be generated).
    uint32_t createUniformBuffer(std::span<const float> data, BufferUsageFlags usage, std::string_view label = {}) const;
    /// A `GL_UNIFORM_BUFFER` with the given raw bytes.
    uint32_t createUniformBuffer(std::span<const uint8_t> data, BufferUsageFlags usage,
                               std::string_view label = {}) const;
    /// A `GL_UNIFORM_BUFFER` of `byteSize` uninitialised bytes.
    uint32_t createUniformBuffer(size_t byteSize, BufferUsageFlags usage, std::string_view label = {}) const;

    /// Overwrites part of a buffer (`glBufferSubData`). The caller makes sure the range is inside the buffer.
    /// @param type       the kind the buffer was created as.
    /// @param buffer     buffer name.
    /// @param byteOffset start of the range in bytes.
    /// @param data       the bytes to write.
    /// @param byteSize   number of bytes to write, greater than zero.
    /// @throws std::invalid_argument for a null buffer, null data or an empty range, std::runtime_error if GL fails.
    void updateBuffer(BufferType type, uint32_t buffer, size_t byteOffset, const void* data, size_t byteSize) const;

    /// `updateVertexBuffer` of ris-framework-ts: writes floats of `data` into a `GL_ARRAY_BUFFER`. Like the
    /// WebGL `bufferSubData` with a typed array, `srcOffset` and `srcLength` count floats while `destOffset`
    /// is a byte offset into the buffer.
    /// @param buffer     buffer name.
    /// @param data       source floats.
    /// @param destOffset where to write in the buffer, in bytes.
    /// @param srcOffset  first float of `data` to write.
    /// @param srcLength  number of floats to write; 0 means all floats from `srcOffset` to the end of `data`
    ///                   (the TypeScript version takes `data.length` here, which only fits for a `srcOffset` of 0).
    /// @throws std::out_of_range if `srcOffset + srcLength` is beyond the end of `data`,
    ///         std::invalid_argument for a null buffer or an empty range, std::runtime_error if GL fails.
    void updateVertexBuffer(uint32_t buffer, std::span<const float> data, size_t destOffset = 0, size_t srcOffset = 0,
                            size_t srcLength = 0) const;

    /// Deletes a buffer and sets the name to 0. Does nothing for 0, or when the window (and context) is gone.
    void deleteBuffer(uint32_t& buffer) const noexcept;
};

} // namespace ris::gles
