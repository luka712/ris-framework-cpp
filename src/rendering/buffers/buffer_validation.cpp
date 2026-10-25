#include "ris/rendering/buffers/buffer_validation.hpp"

#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <string>

#include "ris/utilities/align_utilities.hpp"

// Graphics API neutral: this file includes no backend and no graphics API header.

namespace ris::buffer_validation {

namespace {

constexpr uint32_t kind_mask =
    static_cast<uint32_t>(BufferUsageFlags::Vertex | BufferUsageFlags::Index | BufferUsageFlags::Uniform);

} // namespace

BufferUsageFlags resolve_usage(BufferUsageFlags usage, BufferUsageFlags own, const char* what,
                               BufferUsageFlags unsupported, const char* api_name) {
    if ((static_cast<uint32_t>(usage) & static_cast<uint32_t>(unsupported)) != 0) {
        throw std::domain_error(std::string(what) + ": Storage and MapRead buffers are not available in " + api_name);
    }
    if ((static_cast<uint32_t>(usage) & kind_mask & ~static_cast<uint32_t>(own)) != 0) {
        char hex[16];
        std::snprintf(hex, sizeof hex, "%x", static_cast<unsigned>(usage));
        throw std::invalid_argument(std::string(what) + ": usage 0x" + hex
                                    + " contains the flag of another buffer kind");
    }
    return usage | own;
}

BufferUsageFlags resolve_vertex_buffer(size_t byte_size, uint32_t byte_stride, BufferUsageFlags usage,
                                       BufferUsageFlags unsupported, const char* api_name) {
    const BufferUsageFlags resolved = resolve_usage(usage, BufferUsageFlags::Vertex, "VertexBuffer", unsupported,
                                                    api_name);
    if (byte_stride == 0) {
        throw std::invalid_argument("VertexBuffer: byte_stride must be greater than zero");
    }
    if (byte_size == 0 || byte_size % byte_stride != 0) {
        throw std::invalid_argument(std::string("VertexBuffer: data size ") + std::to_string(byte_size)
                                    + " must be greater than zero and a multiple of the stride "
                                    + std::to_string(byte_stride));
    }
    return resolved;
}

BufferUsageFlags resolve_index_buffer(size_t byte_size) {
    if (byte_size == 0) throw std::invalid_argument("IndexBuffer: indices must not be empty");
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
    if (byte_size == 0) throw std::invalid_argument("UniformBuffer: byte_size must be greater than zero");
    return align_utilities::align(byte_size, uniform_buffer_alignment);
}

std::vector<float> pad_uniform_data(std::span<const float> data) {
    if (data.empty()) throw std::invalid_argument("UniformBuffer: data must not be empty");
    std::vector<float> padded(align_utilities::align(data.size_bytes(), uniform_buffer_alignment) / sizeof(float),
                              0.0f);
    std::copy(data.begin(), data.end(), padded.begin());
    return padded;
}

void check_update_allowed(BufferUsageFlags usage, const char* what) {
    if (!has_flag(usage, BufferUsageFlags::CopyDst)) {
        throw std::invalid_argument(std::string(what) + ": the buffer was not created with BufferUsageFlags::CopyDst");
    }
}

void check_update_range(size_t buffer_byte_size, size_t byte_offset, const void* data, size_t size, const char* what) {
    if (!data) throw std::invalid_argument(std::string(what) + ": data must not be null");
    if (byte_offset + size > buffer_byte_size) {
        throw std::out_of_range(std::string(what) + ": range [" + std::to_string(byte_offset) + ", "
                                + std::to_string(byte_offset + size) + ") is outside the buffer (size "
                                + std::to_string(buffer_byte_size) + ")");
    }
}

void check_layout_stride(const VertexBufferLayout& layout, uint32_t buffer_byte_stride, const char* what) {
    if (layout.array_stride != buffer_byte_stride) {
        throw std::invalid_argument(std::string(what) + ": layout stride " + std::to_string(layout.array_stride)
                                    + " differs from the buffer stride " + std::to_string(buffer_byte_stride));
    }
}

} // namespace ris::buffer_validation
