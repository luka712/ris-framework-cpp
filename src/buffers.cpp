#include "ris/buffers.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

#include "internal.hpp"

using namespace ris::detail;

namespace ris {

namespace {

constexpr uint32_t kind_mask = static_cast<uint32_t>(BufferUsageFlags::Vertex | BufferUsageFlags::Index |
                                                     BufferUsageFlags::Uniform);

/// Validates `usage` for a buffer of the kind `own` and returns the flags to store (given flags + own).
BufferUsageFlags resolve_usage(BufferUsageFlags usage, BufferUsageFlags own, const char* what) {
    if (has_flag(usage, BufferUsageFlags::Storage) || has_flag(usage, BufferUsageFlags::MapRead)) {
        throw_error(Result::Unsupported, "%s: Storage and MapRead buffers are not available in OpenGL ES 3.0", what);
    }
    if ((static_cast<uint32_t>(usage) & kind_mask & ~static_cast<uint32_t>(own)) != 0) {
        throw_error(Result::InvalidArgument, "%s: usage 0x%x contains the flag of another buffer kind", what,
                    static_cast<unsigned>(usage));
    }
    return usage | own;
}

/// Validates the arguments of a vertex buffer and returns the usage to store.
BufferUsageFlags resolve_vertex(size_t byte_size, uint32_t byte_stride, BufferUsageFlags usage) {
    const BufferUsageFlags resolved = resolve_usage(usage, BufferUsageFlags::Vertex, "VertexBuffer");
    if (byte_stride == 0) {
        throw_error(Result::InvalidArgument, "VertexBuffer: byte_stride must be greater than zero");
    }
    if (byte_size == 0 || byte_size % byte_stride != 0) {
        throw_error(Result::InvalidArgument,
                    "VertexBuffer: data size %zu must be greater than zero and a multiple of the stride %u", byte_size,
                    byte_stride);
    }
    return resolved;
}

size_t align_up_16(size_t bytes) { return (bytes + 15) & ~size_t{15}; }

uint32_t attribute_components(VertexFormat f) {
    switch (f) {
        case VertexFormat::Float32: return 1;
        case VertexFormat::Float32x2: return 2;
        case VertexFormat::Float32x3: return 3;
        case VertexFormat::Float32x4: return 4;
        case VertexFormat::Float32x16: return 16;
    }
    return 0;
}

} // namespace

uint32_t vertex_format_byte_size(VertexFormat format) noexcept { return attribute_components(format) * 4u; }

// ---- VertexBufferLayout ---------------------------------------------------------------------

VertexBufferLayout VertexBufferLayout::create_float3_float2() {
    return {20, VertexStepMode::Vertex,
            {{VertexFormat::Float32x3, 0, 0}, {VertexFormat::Float32x2, 12, 1}}};
}

VertexBufferLayout VertexBufferLayout::create_float3_float4_float2() {
    return {36, VertexStepMode::Vertex,
            {{VertexFormat::Float32x3, 0, 0}, {VertexFormat::Float32x4, 12, 1}, {VertexFormat::Float32x2, 28, 2}}};
}

VertexBufferLayout VertexBufferLayout::create_float3_float4_float2_float3() {
    return {48, VertexStepMode::Vertex,
            {{VertexFormat::Float32x3, 0, 0},
             {VertexFormat::Float32x4, 12, 1},
             {VertexFormat::Float32x2, 28, 2},
             {VertexFormat::Float32x3, 36, 3}}};
}

std::vector<VertexAttrib> VertexBufferLayout::to_vertex_attribs(uint32_t buffer_slot) const {
    std::vector<VertexAttrib> out;
    out.reserve(attributes.size());
    for (const VertexAttribute& a : attributes) {
        const uint32_t size = vertex_format_byte_size(a.format);
        if (size == 0 || a.offset > array_stride || size > array_stride - a.offset) {
            throw_error(Result::InvalidArgument,
                        "VertexBufferLayout: attribute at location %u (offset %u, %u bytes) does not fit the stride %u",
                        a.shader_location, a.offset, size, array_stride);
        }
        // a mat4 is four vec4 columns on consecutive locations, the others are a single attribute
        const uint32_t columns = a.format == VertexFormat::Float32x16 ? 4 : 1;
        for (uint32_t c = 0; c < columns; ++c) {
            VertexAttrib v;
            v.location = a.shader_location + c;
            v.components = columns == 4 ? 4 : attribute_components(a.format);
            v.type = AttribType::Float;
            v.offset = a.offset + c * 16;
            v.buffer_slot = buffer_slot;
            v.divisor = step_mode == VertexStepMode::Instance ? 1u : 0u;
            out.push_back(v);
        }
    }
    return out;
}

// ---- BufferBase -----------------------------------------------------------------------------

BufferBase::BufferBase(BufferType type, BufferUsageFlags usage, const void* data, size_t byte_size, std::string label)
    : buffer_(type, has_flag(usage, BufferUsageFlags::CopyDst) ? BufferUsage::Dynamic : BufferUsage::Static, data,
              byte_size),
      usage_(usage),
      label_(std::move(label)) {}

void BufferBase::write(size_t byte_offset, const void* data, size_t size, const char* what) {
    if (!has_flag(usage_, BufferUsageFlags::CopyDst)) {
        throw_error(Result::InvalidArgument, "%s: the buffer was not created with BufferUsageFlags::CopyDst", what);
    }
    // Buffer::update validates the range (and the pointer) and does nothing if it throws.
    buffer_.update(byte_offset, data, size);
}

// ---- VertexBuffer ---------------------------------------------------------------------------

VertexBuffer::VertexBuffer(const void* data, size_t byte_size, uint32_t byte_stride, BufferUsageFlags usage,
                           std::string label)
    : BufferBase(BufferType::Vertex, resolve_vertex(byte_size, byte_stride, usage), data, byte_size, std::move(label)),
      byte_stride_(byte_stride) {}

VertexBuffer::VertexBuffer(std::span<const float> data, uint32_t byte_stride, BufferUsageFlags usage,
                           std::string label)
    : VertexBuffer(data.data(), data.size_bytes(), byte_stride, usage, std::move(label)) {}

VertexBuffer VertexBuffer::with_byte_size(size_t byte_size, uint32_t byte_stride, BufferUsageFlags usage,
                                          std::string label) {
    return VertexBuffer(nullptr, byte_size, byte_stride, usage, std::move(label));
}

void VertexBuffer::update(std::span<const float> data, size_t byte_offset) {
    write(byte_offset, data.data(), data.size_bytes(), "VertexBuffer::update");
}

// ---- IndexBuffer ----------------------------------------------------------------------------

namespace {
BufferUsageFlags resolve_index(size_t byte_size) {
    if (byte_size == 0) throw_error(Result::InvalidArgument, "IndexBuffer: indices must not be empty");
    return BufferUsageFlags::Index;
}
} // namespace

IndexBuffer::IndexBuffer(const void* data, size_t byte_size, IndexBufferType type, std::string label)
    : BufferBase(BufferType::Index, resolve_index(byte_size), data, byte_size, std::move(label)), type_(type) {}

IndexBuffer::IndexBuffer(std::span<const uint16_t> indices, std::string label)
    : IndexBuffer(indices.data(), indices.size_bytes(), IndexBufferType::UInt16, std::move(label)) {}

namespace {
/// Narrows 32 bit indices to 16 bit when all fit, like the ris-framework-ts index buffer does.
std::vector<uint16_t> narrow_indices(std::span<const uint32_t> indices) {
    std::vector<uint16_t> out;
    const bool fits = std::all_of(indices.begin(), indices.end(), [](uint32_t i) { return i <= 0xFFFFu; });
    if (fits) out.assign(indices.begin(), indices.end());
    return out;
}
} // namespace

IndexBuffer::IndexBuffer(std::span<const uint32_t> indices, std::string label)
    : IndexBuffer(
          [&]() -> IndexBuffer {
              const std::vector<uint16_t> narrow = narrow_indices(indices);
              if (!narrow.empty()) return IndexBuffer(std::span<const uint16_t>(narrow), std::move(label));
              return IndexBuffer(indices.data(), indices.size_bytes(), IndexBufferType::UInt32, std::move(label));
          }()) {}

IndexBuffer IndexBuffer::create_uint32(std::span<const uint32_t> indices, std::string label) {
    return IndexBuffer(indices.data(), indices.size_bytes(), IndexBufferType::UInt32, std::move(label));
}

// ---- UniformBuffer --------------------------------------------------------------------------

UniformBuffer::UniformBuffer(const void* data, size_t byte_size, BufferUsageFlags usage, std::string label)
    : BufferBase(BufferType::Uniform, resolve_usage(usage, BufferUsageFlags::Uniform, "UniformBuffer"), data,
                 byte_size, std::move(label)) {}

UniformBuffer::UniformBuffer(std::span<const float> data, BufferUsageFlags usage, std::string label)
    : UniformBuffer(
          [&]() -> UniformBuffer {
              if (data.empty()) throw_error(Result::InvalidArgument, "UniformBuffer: data must not be empty");
              std::vector<float> padded(align_up_16(data.size_bytes()) / sizeof(float), 0.0f);
              std::copy(data.begin(), data.end(), padded.begin());
              return UniformBuffer(padded.data(), padded.size() * sizeof(float), usage, std::move(label));
          }()) {}

UniformBuffer UniformBuffer::with_byte_size(size_t byte_size, BufferUsageFlags usage, std::string label) {
    if (byte_size == 0) throw_error(Result::InvalidArgument, "UniformBuffer: byte_size must be greater than zero");
    const std::vector<float> zeros(align_up_16(byte_size) / sizeof(float), 0.0f);
    return UniformBuffer(zeros.data(), zeros.size() * sizeof(float), usage, std::move(label));
}

void UniformBuffer::update(std::span<const float> data, size_t byte_offset) {
    write(byte_offset, data.data(), data.size_bytes(), "UniformBuffer::update");
}

void UniformBuffer::update(const Mat4& matrix, size_t byte_offset) {
    write(byte_offset, glm::value_ptr(matrix), sizeof(float) * 16, "UniformBuffer::update");
}

void UniformBuffer::update(const Vec4& vector, size_t byte_offset) {
    write(byte_offset, glm::value_ptr(vector), sizeof(float) * 4, "UniformBuffer::update");
}

// ---- BufferFactory --------------------------------------------------------------------------

std::unique_ptr<IndexBuffer> BufferFactory::create_index_buffer(std::span<const uint32_t> indices,
                                                                std::string label) const {
    return std::make_unique<IndexBuffer>(indices, std::move(label));
}

std::unique_ptr<VertexBuffer> BufferFactory::create_vertex_buffer(std::span<const float> data, uint32_t byte_stride,
                                                                  BufferUsageFlags usage, std::string label) const {
    return std::make_unique<VertexBuffer>(data, byte_stride, usage, std::move(label));
}

std::unique_ptr<UniformBuffer> BufferFactory::create_uniform_buffer(std::span<const float> data,
                                                                    BufferUsageFlags usage, std::string label) const {
    return std::make_unique<UniformBuffer>(data, usage, std::move(label));
}

// ---- helpers --------------------------------------------------------------------------------

VertexArrayDesc make_vertex_array_desc(const VertexBuffer& vertex_buffer, const VertexBufferLayout& layout,
                                       const IndexBuffer* index_buffer) {
    if (layout.array_stride != vertex_buffer.byte_stride()) {
        throw_error(Result::InvalidArgument, "make_vertex_array_desc: layout stride %u differs from the buffer stride %u",
                    layout.array_stride, vertex_buffer.byte_stride());
    }
    VertexArrayDesc desc;
    desc.attribs = layout.to_vertex_attribs(0);
    desc.vertex_buffers[0] = vertex_buffer.binding();
    if (index_buffer) {
        desc.index_buffer = &index_buffer->buffer();
        desc.index_type = index_buffer->index_type();
    }
    return desc;
}

} // namespace ris
