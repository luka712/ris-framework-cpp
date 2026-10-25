#include "ris/rendering/buffers/buffer_validation.hpp"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <string>

#include "ris/utilities/align_utilities.hpp"

// Graphics API neutral: this file includes no backend and no graphics API header.

namespace ris::buffer_validation {

namespace {

constexpr uint32_t kind_mask =
    static_cast<uint32_t>(BufferUsageFlags::Vertex | BufferUsageFlags::Index | BufferUsageFlags::Uniform);

[[noreturn]] void fail(Result code, const char* fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

void fail(Result code, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    va_list copy;
    va_copy(copy, args);
    const int n = std::vsnprintf(nullptr, 0, fmt, copy);
    va_end(copy);
    std::string message(n > 0 ? static_cast<size_t>(n) : 0u, '\0');
    if (n > 0) std::vsnprintf(message.data(), message.size() + 1, fmt, args);
    va_end(args);
    throw Error(code, message);
}

} // namespace

BufferUsageFlags resolve_usage(BufferUsageFlags usage, BufferUsageFlags own, const char* what,
                               BufferUsageFlags unsupported, const char* api_name) {
    if ((static_cast<uint32_t>(usage) & static_cast<uint32_t>(unsupported)) != 0) {
        fail(Result::Unsupported, "%s: Storage and MapRead buffers are not available in %s", what, api_name);
    }
    if ((static_cast<uint32_t>(usage) & kind_mask & ~static_cast<uint32_t>(own)) != 0) {
        fail(Result::InvalidArgument, "%s: usage 0x%x contains the flag of another buffer kind", what,
             static_cast<unsigned>(usage));
    }
    return usage | own;
}

BufferUsageFlags resolve_vertex_buffer(size_t byte_size, uint32_t byte_stride, BufferUsageFlags usage,
                                       BufferUsageFlags unsupported, const char* api_name) {
    const BufferUsageFlags resolved = resolve_usage(usage, BufferUsageFlags::Vertex, "VertexBuffer", unsupported,
                                                    api_name);
    if (byte_stride == 0) {
        fail(Result::InvalidArgument, "VertexBuffer: byte_stride must be greater than zero");
    }
    if (byte_size == 0 || byte_size % byte_stride != 0) {
        fail(Result::InvalidArgument,
             "VertexBuffer: data size %zu must be greater than zero and a multiple of the stride %u", byte_size,
             byte_stride);
    }
    return resolved;
}

BufferUsageFlags resolve_index_buffer(size_t byte_size) {
    if (byte_size == 0) fail(Result::InvalidArgument, "IndexBuffer: indices must not be empty");
    return BufferUsageFlags::Index;
}

IndexBufferType select_index_type(std::span<const uint32_t> indices) noexcept {
    const bool fits = std::all_of(indices.begin(), indices.end(), [](uint32_t i) { return i <= max_uint16_index; });
    return fits ? IndexBufferType::UInt16 : IndexBufferType::UInt32;
}

std::vector<uint16_t> narrow_indices(std::span<const uint32_t> indices) {
    std::vector<uint16_t> out;
    if (select_index_type(indices) == IndexBufferType::UInt16) out.assign(indices.begin(), indices.end());
    return out;
}

size_t padded_uniform_byte_size(size_t byte_size) {
    if (byte_size == 0) fail(Result::InvalidArgument, "UniformBuffer: byte_size must be greater than zero");
    return align_utilities::align(byte_size, uniform_buffer_alignment);
}

std::vector<float> pad_uniform_data(std::span<const float> data) {
    if (data.empty()) fail(Result::InvalidArgument, "UniformBuffer: data must not be empty");
    std::vector<float> padded(align_utilities::align(data.size_bytes(), uniform_buffer_alignment) / sizeof(float),
                              0.0f);
    std::copy(data.begin(), data.end(), padded.begin());
    return padded;
}

void check_update_allowed(BufferUsageFlags usage, const char* what) {
    if (!has_flag(usage, BufferUsageFlags::CopyDst)) {
        fail(Result::InvalidArgument, "%s: the buffer was not created with BufferUsageFlags::CopyDst", what);
    }
}

void check_update_range(size_t buffer_byte_size, size_t byte_offset, const void* data, size_t size, const char* what) {
    if (!data || byte_offset + size > buffer_byte_size) {
        fail(Result::InvalidArgument, "%s: range [%zu, %zu) is outside the buffer (size %zu)", what, byte_offset,
             byte_offset + size, buffer_byte_size);
    }
}

void check_layout_stride(const VertexBufferLayout& layout, uint32_t buffer_byte_stride, const char* what) {
    if (layout.array_stride != buffer_byte_stride) {
        fail(Result::InvalidArgument, "%s: layout stride %u differs from the buffer stride %u", what,
             layout.array_stride, buffer_byte_stride);
    }
}

} // namespace ris::buffer_validation
