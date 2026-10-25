#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "ris/core.hpp"
#include "ris/gles/gles_types.hpp"
#include "ris/graphics.hpp"
#include "ris/rendering/buffers/buffer_usage.hpp"

/// Creating and updating raw GL buffer objects, like `WebGlBufferUtilities` of ris-framework-ts. The
/// Gles buffers (GlesVertexBuffer, GlesIndexBuffer, GlesUniformBuffer) and the low level ris::Buffer are built on these.
///
/// The functions return the plain GL buffer name, which you own and must delete with delete_buffer().
/// They need the window's GL context and throw std::logic_error without one.
///
/// Unlike the TypeScript utilities, which bind `ELEMENT_ARRAY_BUFFER` and unbind it afterwards (which
/// detaches the index buffer of the current vertex array in WebGL, too), index buffers are bound to
/// `GL_COPY_WRITE_BUFFER` while they are filled, so the current vertex array is never touched. No target is
/// left bound.
namespace ris::gles::buffer_utilities {

/// Creates a buffer of any kind.
/// @param type      the kind of buffer, selects the binding target (see converter::convert_buffer_target).
/// @param data      initial contents, or null for uninitialised storage of `byte_size` bytes.
/// @param byte_size size in bytes, greater than zero.
/// @param usage     the `glBufferData` usage hint, see converter::convert_buffer_usage.
/// @param label     optional debug label, see debug_label_utilities.
/// @return the buffer name, never 0.
/// @throws std::invalid_argument for a zero size, std::runtime_error if GL reports an error (the buffer is
///         deleted in that case).
RIS_API GlUint create_buffer(BufferType type, const void* data, size_t byte_size, GlEnum usage,
                             std::string_view label = {});

/// `createVertexBuffer` of ris-framework-ts: a `GL_ARRAY_BUFFER` with the given floats.
/// @param data  vertex data, must not be empty.
/// @param usage usage flags, only `CopyDst` matters (dynamic or static draw).
RIS_API GlUint create_vertex_buffer(std::span<const float> data, BufferUsageFlags usage,
                                    std::string_view label = {});
/// A `GL_ARRAY_BUFFER` of `byte_size` uninitialised bytes (the `number` overload of the TypeScript function).
RIS_API GlUint create_vertex_buffer(size_t byte_size, BufferUsageFlags usage, std::string_view label = {});

/// `createIndexBuffer` of ris-framework-ts: an element array buffer with 16 bit indices.
RIS_API GlUint create_index_buffer(std::span<const uint16_t> indices, BufferUsageFlags usage,
                                   std::string_view label = {});
/// An element array buffer with 32 bit indices.
RIS_API GlUint create_index_buffer(std::span<const uint32_t> indices, BufferUsageFlags usage,
                                   std::string_view label = {});

/// `createUniformBuffer` of ris-framework-ts: a `GL_UNIFORM_BUFFER` with the given floats. The data is not
/// padded here (GlesUniformBuffer does that).
RIS_API GlUint create_uniform_buffer(std::span<const float> data, BufferUsageFlags usage,
                                     std::string_view label = {});
/// A `GL_UNIFORM_BUFFER` of `byte_size` uninitialised bytes.
RIS_API GlUint create_uniform_buffer(size_t byte_size, BufferUsageFlags usage, std::string_view label = {});

/// Overwrites part of a buffer (`glBufferSubData`). The caller makes sure the range is inside the buffer.
/// @param type       the kind the buffer was created as.
/// @param buffer     buffer name.
/// @param byte_offset start of the range in bytes.
/// @throws std::invalid_argument for a null buffer, null data or an empty range, std::runtime_error if GL fails.
RIS_API void update_buffer(BufferType type, GlUint buffer, size_t byte_offset, const void* data, size_t byte_size);

/// `updateVertexBuffer` of ris-framework-ts: writes `data` into a `GL_ARRAY_BUFFER`.
/// @param dest_byte_offset where to write in the buffer, in bytes.
RIS_API void update_vertex_buffer(GlUint buffer, std::span<const float> data, size_t dest_byte_offset = 0);

/// Deletes a buffer and sets the name to 0. Does nothing for 0, or when the window (and context) is gone.
RIS_API void delete_buffer(GlUint& buffer) noexcept;

} // namespace ris::gles::buffer_utilities
