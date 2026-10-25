#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "core.hpp"
#include "graphics.hpp"
#include "math.hpp"

// TS-style typed GPU buffers, mirroring ris-framework-ts `IBuffer`, `IVertexBuffer`, `IIndexBuffer`,
// `IUniformBuffer`, `IBufferFactory` and `IVertexBufferLayout`.
//
// They are thin RAII wrappers over the low level ris::Buffer of graphics.hpp, so all of its rules apply:
//  - a ris::Window must exist (its OpenGL ES context is current) while a buffer is created, updated or
//    destroyed, and everything must run on the window's thread;
//  - buffers are move-only and own their GL buffer object, which is deleted on destruction;
//  - failures throw ris::Error (argument problems are thrown before any GL call is made, so they do not
//    need a window).
// There is no C API for these types, use ris_buffer_* of ris.h from C.

namespace ris {

// ---- usage and formats ----------------------------------------------------------------------

/// Usage flags of a buffer, like `BufferUsage` of ris-framework-ts. Combine with operator|.
///
/// (The low level `ris::BufferUsage` of graphics.hpp is the GL update frequency hint, a different thing.)
/// Only `Vertex`, `Index`, `Uniform` and `CopyDst` can be honoured with OpenGL ES 3.0.
enum class BufferUsageFlags : uint32_t {
    None = 0,       ///< No usage defined.
    Vertex = 1,     ///< Vertex buffer of a draw operation (GL_ARRAY_BUFFER).
    Index = 2,      ///< Index buffer of a draw operation (GL_ELEMENT_ARRAY_BUFFER).
    Uniform = 4,    ///< Uniform buffer of a draw operation (GL_UNIFORM_BUFFER).
    Storage = 8,    ///< Storage buffer. Not available in OpenGL ES 3.0: creating one throws Result::Unsupported.
    MapRead = 16,   ///< Buffer can be read back by the CPU. Not available in OpenGL ES 3.0: throws Result::Unsupported.
    CopyDst = 32    ///< Buffer is written after creation (update()). Selects GL_DYNAMIC_DRAW, otherwise GL_STATIC_DRAW.
};

constexpr BufferUsageFlags operator|(BufferUsageFlags a, BufferUsageFlags b) noexcept {
    return static_cast<BufferUsageFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
constexpr BufferUsageFlags operator&(BufferUsageFlags a, BufferUsageFlags b) noexcept {
    return static_cast<BufferUsageFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
constexpr BufferUsageFlags& operator|=(BufferUsageFlags& a, BufferUsageFlags b) noexcept { return a = a | b; }
/// True if every flag of `flag` is set in `flags`.
constexpr bool has_flag(BufferUsageFlags flags, BufferUsageFlags flag) noexcept {
    return (flags & flag) == flag;
}

/// The element type of an IndexBuffer, like `IndexBufferType` of ris-framework-ts.
enum class IndexBufferType : uint32_t {
    UInt16 = 0,   ///< 2 byte unsigned indices.
    UInt32 = 1    ///< 4 byte unsigned indices.
};

/// How a vertex buffer is stepped through while drawing, like `VertexStepMode` of ris-framework-ts.
enum class VertexStepMode : uint32_t {
    Vertex = 0,    ///< One element per vertex (attribute divisor 0).
    Instance = 1   ///< One element per instance (attribute divisor 1).
};

/// The format of one vertex attribute, like `VertexFormat` of ris-framework-ts. All formats are 32 bit floats.
enum class VertexFormat : uint32_t {
    Float32 = 0,     ///< float
    Float32x2 = 1,   ///< vec2
    Float32x3 = 2,   ///< vec3
    Float32x4 = 3,   ///< vec4
    Float32x16 = 4   ///< mat4, consumes four consecutive shader locations (one vec4 column each)
};

/// Size in bytes of a vertex format (4, 8, 12, 16 or 64).
RIS_API uint32_t vertex_format_byte_size(VertexFormat format) noexcept;

/// One attribute of a vertex, like `VertexAttribute` of ris-framework-ts.
struct VertexAttribute {
    VertexFormat format = VertexFormat::Float32;
    uint32_t offset = 0;            ///< Byte offset from the start of the vertex.
    uint32_t shader_location = 0;   ///< `layout(location = N)` in the vertex shader.
};

/// Describes how the vertices in a VertexBuffer are laid out, like `IVertexBufferLayout` of ris-framework-ts.
/// Plain data: it does not reference any GL object.
struct RIS_API VertexBufferLayout {
    uint32_t array_stride = 0;                       ///< Size of one vertex in bytes.
    VertexStepMode step_mode = VertexStepMode::Vertex;
    std::vector<VertexAttribute> attributes;

    /// position (vec3, location 0) + uv (vec2, location 1): 20 bytes.
    static VertexBufferLayout create_float3_float2();
    /// position (vec3, 0) + colour (vec4, 1) + uv (vec2, 2): 36 bytes.
    static VertexBufferLayout create_float3_float4_float2();
    /// position (vec3, 0) + colour (vec4, 1) + uv (vec2, 2) + normal (vec3, 3): 48 bytes.
    static VertexBufferLayout create_float3_float4_float2_float3();

    /// Converts the attributes to the low level description used by VertexArrayDesc.
    /// A `Float32x16` attribute expands to four vec4 attributes on consecutive locations. The step mode
    /// becomes the divisor (Instance = 1).
    /// @param buffer_slot index into VertexArrayDesc::vertex_buffers that the attributes read from.
    /// @throws Error(InvalidArgument) if an attribute does not fit into `array_stride`.
    std::vector<VertexAttrib> to_vertex_attribs(uint32_t buffer_slot = 0) const;
};

// ---- buffers --------------------------------------------------------------------------------

/// Common base of VertexBuffer, IndexBuffer and UniformBuffer, like `IBuffer` of ris-framework-ts.
/// Not constructible by users. Owns one GL buffer object, move-only; a moved-from buffer is empty and
/// must not be used again. Requires the window's GL context for every operation (see the file comment).
class RIS_API BufferBase {
public:
    /// The debug label given at creation, empty if none. The label is kept on the CPU only.
    const std::string& label() const noexcept { return label_; }
    /// Size of the GPU storage in bytes. Uniform buffers report their padded size.
    size_t byte_size() const noexcept { return buffer_.size(); }
    /// The usage flags: the flags given at creation plus the flag of the buffer kind (e.g. Index).
    BufferUsageFlags usage() const noexcept { return usage_; }
    /// The OpenGL buffer name, for interop with native code. 0 for a moved-from buffer.
    uint32_t gl_handle() const noexcept { return buffer_.gl_handle(); }
    /// The underlying low level buffer, e.g. for VertexArrayDesc::index_buffer. Owned by this object and
    /// valid only while it lives.
    const Buffer& buffer() const noexcept { return buffer_; }

    BufferBase(const BufferBase&) = delete;
    BufferBase& operator=(const BufferBase&) = delete;

protected:
    BufferBase(BufferType type, BufferUsageFlags usage, const void* data, size_t byte_size, std::string label);
    BufferBase(BufferBase&&) noexcept = default;
    BufferBase& operator=(BufferBase&&) noexcept = default;
    ~BufferBase() = default;

    /// Writes `size` bytes at `byte_offset`. Throws Error(InvalidArgument) if outside the buffer, or
    /// if the buffer was not created with BufferUsageFlags::CopyDst.
    void write(size_t byte_offset, const void* data, size_t size, const char* what);

private:
    Buffer buffer_;
    BufferUsageFlags usage_;
    std::string label_;
};

/// A vertex buffer of 32 bit floats, like `IVertexBuffer` of ris-framework-ts.
class RIS_API VertexBuffer : public BufferBase {
public:
    /// Creates the buffer and uploads `data`.
    /// @param data     floats to upload (copied). Must not be empty and its byte size must be a multiple
    ///                 of `byte_stride`.
    /// @param byte_stride size of one vertex in bytes, greater than zero.
    /// @param usage    usage flags. Add BufferUsageFlags::CopyDst if update() will be used. Flags of other
    ///                 buffer kinds (Index, Uniform) are rejected.
    /// @param label    optional debug label.
    /// @throws Error(InvalidArgument) for bad arguments, Error(Unsupported) for Storage / MapRead,
    ///         Error(NotInitialized) without a window, Error(Graphics) if GL fails.
    VertexBuffer(std::span<const float> data, uint32_t byte_stride,
                 BufferUsageFlags usage = BufferUsageFlags::Vertex, std::string label = {});

    /// Creates a buffer with uninitialised contents of `byte_size` bytes (a multiple of `byte_stride`).
    /// Fill it with update(), so pass BufferUsageFlags::CopyDst. Same errors as the constructor.
    static VertexBuffer with_byte_size(size_t byte_size, uint32_t byte_stride,
                                       BufferUsageFlags usage = BufferUsageFlags::Vertex | BufferUsageFlags::CopyDst,
                                       std::string label = {});

    /// Number of vertices: byte_size() / byte_stride().
    uint32_t vertex_count() const noexcept { return static_cast<uint32_t>(byte_size() / byte_stride_); }
    /// Size of one vertex in bytes.
    uint32_t byte_stride() const noexcept { return byte_stride_; }

    /// Overwrites part of the buffer with `data` (copied, the call does not keep the pointer).
    /// @param byte_offset where to start writing, in bytes.
    /// @throws Error(InvalidArgument) if the buffer lacks CopyDst or [byte_offset, byte_offset + data
    ///         bytes) is outside the buffer. Nothing is written in that case.
    void update(std::span<const float> data, size_t byte_offset = 0);

    /// The binding to put into VertexArrayDesc::vertex_buffers. Refers to this buffer, which must outlive
    /// the VertexArray.
    VertexBufferBinding binding() const noexcept { return {&buffer(), byte_stride_}; }

private:
    VertexBuffer(const void* data, size_t byte_size, uint32_t byte_stride, BufferUsageFlags usage, std::string label);
    uint32_t byte_stride_ = 0;
};

/// An index buffer, like `IIndexBuffer` of ris-framework-ts. The data is immutable after creation.
class RIS_API IndexBuffer : public BufferBase {
public:
    /// Creates the buffer from 32 bit indices. Like ris-framework-ts it stores 16 bit indices when every
    /// index is <= 65535 and 32 bit indices otherwise, see type().
    /// @param indices the indices (copied), must not be empty.
    /// @param label   optional debug label.
    /// @throws Error(InvalidArgument) if empty, Error(NotInitialized) without a window,
    ///         Error(Graphics) if GL fails.
    explicit IndexBuffer(std::span<const uint32_t> indices, std::string label = {});
    /// Creates the buffer from 16 bit indices, stored as UInt16. Same errors as above.
    explicit IndexBuffer(std::span<const uint16_t> indices, std::string label = {});

    /// Creates the buffer from 32 bit indices and always stores them as UInt32.
    static IndexBuffer create_uint32(std::span<const uint32_t> indices, std::string label = {});

    /// The stored element type.
    IndexBufferType type() const noexcept { return type_; }
    /// The element type as the low level IndexType, for VertexArrayDesc::index_type.
    IndexType index_type() const noexcept {
        return type_ == IndexBufferType::UInt16 ? IndexType::UInt16 : IndexType::UInt32;
    }
    /// Size of one index in bytes, 2 or 4.
    uint32_t element_byte_size() const noexcept { return type_ == IndexBufferType::UInt16 ? 2u : 4u; }
    /// Number of indices.
    uint32_t indices_count() const noexcept { return static_cast<uint32_t>(byte_size() / element_byte_size()); }

private:
    IndexBuffer(const void* data, size_t byte_size, IndexBufferType type, std::string label);
    IndexBufferType type_ = IndexBufferType::UInt16;
};

/// A uniform buffer, like `IUniformBuffer` of ris-framework-ts. Bind it to a uniform block binding point
/// with bind_base() and Shader::bind_uniform_block(). Lay the data out according to std140.
class RIS_API UniformBuffer : public BufferBase {
public:
    /// Creates the buffer and uploads `data`, zero padded to a multiple of 16 bytes (4 floats), like
    /// `AlignUtilities.alignArray(data, 4)` in ris-framework-ts.
    /// @param data   floats to upload (copied), must not be empty.
    /// @param usage  usage flags, add BufferUsageFlags::CopyDst if update() will be used. Flags of other
    ///               buffer kinds (Vertex, Index) are rejected.
    /// @param label  optional debug label.
    /// @throws Error(InvalidArgument) for bad arguments, Error(Unsupported) for Storage / MapRead,
    ///         Error(NotInitialized) without a window, Error(Graphics) if GL fails.
    explicit UniformBuffer(std::span<const float> data,
                           BufferUsageFlags usage = BufferUsageFlags::Uniform | BufferUsageFlags::CopyDst,
                           std::string label = {});

    /// Creates a zero filled buffer of `byte_size` bytes rounded up to a multiple of 16. Same errors as
    /// the constructor.
    static UniformBuffer with_byte_size(size_t byte_size,
                                        BufferUsageFlags usage = BufferUsageFlags::Uniform | BufferUsageFlags::CopyDst,
                                        std::string label = {});

    /// Overwrites part of the buffer with `data` (copied).
    /// @param byte_offset where to start writing, in bytes. Must be inside the (padded) buffer.
    /// @throws Error(InvalidArgument) if the buffer lacks CopyDst or the range is outside the buffer.
    ///         Nothing is written in that case.
    void update(std::span<const float> data, size_t byte_offset = 0);
    /// Writes a column-major matrix (64 bytes).
    void update(const Mat4& matrix, size_t byte_offset = 0);
    /// Writes a vector (16 bytes).
    void update(const Vec4& vector, size_t byte_offset = 0);

    /// Binds the whole buffer to uniform block binding point `binding` (glBindBufferBase).
    void bind_base(uint32_t binding) const { buffer().bind_base(binding); }

private:
    UniformBuffer(const void* data, size_t byte_size, BufferUsageFlags usage, std::string label);
};

/// Creates buffers, like `IBufferFactory` of ris-framework-ts. Stateless; the buffers are returned in
/// `std::unique_ptr`s because ris-framework-ts hands out interfaces. The same rules and errors as the
/// buffer constructors apply.
class RIS_API BufferFactory {
public:
    /// See IndexBuffer::IndexBuffer(std::span<const uint32_t>, std::string).
    std::unique_ptr<IndexBuffer> create_index_buffer(std::span<const uint32_t> indices, std::string label = {}) const;
    /// See VertexBuffer::VertexBuffer.
    std::unique_ptr<VertexBuffer> create_vertex_buffer(std::span<const float> data, uint32_t byte_stride,
                                                       BufferUsageFlags usage = BufferUsageFlags::Vertex,
                                                       std::string label = {}) const;
    /// See UniformBuffer::UniformBuffer.
    std::unique_ptr<UniformBuffer> create_uniform_buffer(
        std::span<const float> data,
        BufferUsageFlags usage = BufferUsageFlags::Uniform | BufferUsageFlags::CopyDst, std::string label = {}) const;
};

/// Builds the description of a VertexArray that reads `vertex_buffer` through `layout` (buffer slot 0)
/// and optionally draws indexed with `index_buffer`.
/// The returned description holds pointers to the buffers, so they must outlive the VertexArray created
/// from it.
/// @throws Error(InvalidArgument) if `layout.array_stride` differs from the buffer's stride or an
///         attribute does not fit into the stride.
RIS_API VertexArrayDesc make_vertex_array_desc(const VertexBuffer& vertex_buffer, const VertexBufferLayout& layout,
                                               const IndexBuffer* index_buffer = nullptr);

} // namespace ris
