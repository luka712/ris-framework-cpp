#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "ris/core.hpp"
#include "ris/rendering/buffers/buffer_usage.hpp"
#include "ris/rendering/buffers/index_buffer_type.hpp"
#include "ris/rendering/buffers/vertex_buffer_layout.hpp"

/// Argument checks and data preparation shared by every buffer backend. Graphics API neutral: no device is
/// touched, so the functions need no window and a backend (GlesBufferFactory, or a future Vulkan / Metal /
/// D3D one) calls them before it creates anything. That way all backends reject the same bad arguments with
/// the same errors and store the same data (padding, index type).
///
/// All checks throw standard exceptions (see below). `what` is the name used in the message, e.g. "VertexBuffer".
namespace ris::buffer_validation {

/// Uniform buffers are padded to a multiple of this many bytes (4 floats, std140 vec4 alignment).
inline constexpr size_t uniform_buffer_alignment = 16;
/// Largest index that is stored as 16 bit by IndexBufferType selection.
inline constexpr uint32_t max_uint16_index = 0xFFFFu;

/// Validates `usage` for a buffer of the kind `own` (one of Vertex, Index, Uniform) and returns the flags to
/// store (the given flags plus `own`).
/// @param unsupported flags the backend cannot honour, normally `Storage | MapRead` for OpenGL ES 3.0.
/// @param api_name    name of the backend for the message, e.g. "OpenGL ES 3.0".
/// @throws std::domain_error if a flag of `unsupported` is set, std::invalid_argument if the flag of another
///         buffer kind is set.
RIS_API BufferUsageFlags resolve_usage(BufferUsageFlags usage, BufferUsageFlags own, const char* what,
                                       BufferUsageFlags unsupported, const char* api_name);

/// Validates the arguments of a vertex buffer and returns the usage to store (see resolve_usage).
/// @throws std::invalid_argument if `byte_stride` is zero, `byte_size` is zero or not a multiple of the
///         stride, or the usage has the flag of another kind; std::domain_error for unsupported flags.
RIS_API BufferUsageFlags resolve_vertex_buffer(size_t byte_size, uint32_t byte_stride, BufferUsageFlags usage,
                                               BufferUsageFlags unsupported, const char* api_name);

/// Validates the size of an index buffer and returns the usage to store (always `Index`).
/// @throws std::invalid_argument if `byte_size` is zero.
RIS_API BufferUsageFlags resolve_index_buffer(size_t byte_size);

/// The element type an index buffer stores for `indices`: UInt16 if every index is <= 65535 (also for an
/// empty list), otherwise UInt32. Like ris-framework-ts.
RIS_API IndexBufferType select_index_type(std::span<const uint32_t> indices) noexcept;
/// The indices narrowed to 16 bit. Only meaningful if select_index_type() is UInt16; otherwise the result is
/// empty.
RIS_API std::vector<uint16_t> narrow_indices(std::span<const uint32_t> indices);

/// Size in bytes of a uniform buffer for `byte_size` bytes of data: rounded up to uniform_buffer_alignment.
/// @throws std::invalid_argument if `byte_size` is zero.
RIS_API size_t padded_uniform_byte_size(size_t byte_size);
/// Copies `data` into a vector padded with zeros to uniform_buffer_alignment, like
/// `AlignUtilities.alignArray(data, 4)` in ris-framework-ts.
/// @throws std::invalid_argument if `data` is empty.
RIS_API std::vector<float> pad_uniform_data(std::span<const float> data);

/// Checks that a buffer with `usage` may be written after creation.
/// @throws std::invalid_argument if it lacks BufferUsageFlags::CopyDst.
RIS_API void check_update_allowed(BufferUsageFlags usage, const char* what);
/// Checks that [byte_offset, byte_offset + size) lies inside a buffer of `buffer_byte_size` bytes and that
/// `data` is not null.
/// @throws std::invalid_argument if `data` is null, std::out_of_range if the range is outside the buffer.
RIS_API void check_update_range(size_t buffer_byte_size, size_t byte_offset, const void* data, size_t size,
                                const char* what);

/// Checks that `layout.array_stride` equals the stride of the vertex buffer it is used with.
/// @param what name of the calling function, used in the message.
/// @throws std::invalid_argument if they differ.
RIS_API void check_layout_stride(const VertexBufferLayout& layout, uint32_t buffer_byte_stride, const char* what);

} // namespace ris::buffer_validation
