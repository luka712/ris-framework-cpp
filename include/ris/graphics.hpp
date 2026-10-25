#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "core.hpp"
#include "math.hpp"
#include "ris/gles/texture/gles_texture2d.hpp"
#include "ris/rendering/primitive/cull_mode.hpp"

// OpenGL ES 3 graphics objects. A ris::Window must exist (its context is current) while any of
// these are created, used or destroyed, and everything must run on the window's thread.
// All objects are RAII, move-only, and throw standard exceptions (std::invalid_argument, std::runtime_error, ...) on failure.

namespace ris {

// ---- state ----------------------------------------------------------------------------------

enum class ClearFlags : uint32_t { Color = 1u << 0, Depth = 1u << 1, Stencil = 1u << 2 };
constexpr ClearFlags operator|(ClearFlags a, ClearFlags b) noexcept {
    return static_cast<ClearFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

enum class BlendMode : uint32_t {
    None = 0,
    Alpha = 1,          ///< src*a + dst*(1-a)
    Premultiplied = 2,  ///< src + dst*(1-a)
    Additive = 3,
    Multiply = 4
};

enum class CompareFunc : uint32_t { Never, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always };

// CullMode is graphics API neutral, see ris/rendering/primitive/cull_mode.hpp (included above).

namespace gfx {
RIS_API void viewport(int32_t x, int32_t y, int32_t width, int32_t height);
RIS_API void set_scissor(bool enabled, int32_t x = 0, int32_t y = 0, int32_t width = 0, int32_t height = 0);
RIS_API void clear(float r, float g, float b, float a, ClearFlags flags = ClearFlags::Color | ClearFlags::Depth);
RIS_API void set_blend_mode(BlendMode mode);
RIS_API void set_depth_state(bool test_enabled, bool write_enabled, CompareFunc func = CompareFunc::Less);
/// Counter clockwise triangles are front facing.
RIS_API void set_cull_mode(CullMode mode);
/// Reads RGBA8 pixels from the current framebuffer (origin bottom left).
RIS_API std::vector<uint8_t> read_pixels(int32_t x, int32_t y, int32_t width, int32_t height);
} // namespace gfx

// ---- buffers --------------------------------------------------------------------------------

enum class BufferType : uint32_t { Vertex = 0, Index = 1, Uniform = 2 };
enum class BufferUsage : uint32_t {
    Static = 0,   ///< written once, drawn many times
    Dynamic = 1,  ///< updated occasionally
    Stream = 2    ///< updated every frame
};

class RIS_API Buffer {
public:
    /// `data` may be null to allocate uninitialised storage of `size` bytes.
    Buffer(BufferType type, BufferUsage usage, const void* data, size_t size);
    ~Buffer();
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    void update(size_t offset, const void* data, size_t size);
    /// Binds the buffer to a uniform block binding point (uniform buffers only).
    void bind_base(uint32_t binding) const;

    size_t size() const noexcept { return size_; }
    BufferType type() const noexcept { return type_; }
    uint32_t gl_handle() const noexcept { return id_; }

private:
    uint32_t id_ = 0;
    BufferType type_ = BufferType::Vertex;
    size_t size_ = 0;
};

// ---- shaders --------------------------------------------------------------------------------

class RIS_API Shader {
public:
    /// GLSL ES 3.00 (`#version 300 es`) sources. Throws std::runtime_error with the info log.
    Shader(const std::string& vertex_source, const std::string& fragment_source);
    ~Shader();
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;
    /// -1 if there is no such active uniform; setting location -1 is a no-op.
    int32_t uniform_location(const char* name) const;
    int32_t attrib_location(const char* name) const;

    void set_int(int32_t location, int32_t v) const;
    void set_float(int32_t location, float v) const;
    void set_vec2(int32_t location, float x, float y) const;
    void set_vec3(int32_t location, float x, float y, float z) const;
    void set_vec4(int32_t location, float x, float y, float z, float w) const;
    void set_mat4(int32_t location, const float* column_major_16) const;
    // glm overloads
    void set_vec2(int32_t location, const Vec2& v) const { set_vec2(location, v.x, v.y); }
    void set_vec3(int32_t location, const Vec3& v) const { set_vec3(location, v.x, v.y, v.z); }
    void set_vec4(int32_t location, const Vec4& v) const { set_vec4(location, v.x, v.y, v.z, v.w); }
    void set_mat4(int32_t location, const Mat4& m) const { set_mat4(location, glm::value_ptr(m)); }
    /// Assigns a named uniform block to a binding point, see Buffer::bind_base.
    void bind_uniform_block(const char* block_name, uint32_t binding) const;

    uint32_t gl_handle() const noexcept { return program_; }

private:
    uint32_t program_ = 0;
};

// ---- meshes ---------------------------------------------------------------------------------

enum class AttribType : uint32_t { Float = 0, UByte = 1, Byte = 2, UShort = 3, Short = 4, UInt = 5, Int = 6 };
enum class IndexType : uint32_t { None = 0, UInt16 = 1, UInt32 = 2 };
enum class Topology : uint32_t { Triangles = 0, TriangleStrip = 1, TriangleFan = 2, Lines = 3, LineStrip = 4, Points = 5 };

inline constexpr size_t max_vertex_buffers = 4;

struct VertexAttrib {
    uint32_t location = 0;      ///< layout(location = N) in the vertex shader
    uint32_t components = 4;    ///< 1..4
    AttribType type = AttribType::Float;
    bool normalized = false;    ///< integer types only: converted to float in [0,1] / [-1,1]
    uint32_t offset = 0;        ///< byte offset inside the vertex
    uint32_t buffer_slot = 0;   ///< index into VertexArrayDesc::vertex_buffers
    uint32_t divisor = 0;       ///< 0 = per vertex, N = per N instances
    bool as_integer = false;    ///< feed an `ivec` / `uvec` input (integer types only)
};

struct VertexBufferBinding {
    const Buffer* buffer = nullptr;
    uint32_t stride = 0;
};

struct VertexArrayDesc {
    std::vector<VertexAttrib> attribs;
    std::array<VertexBufferBinding, max_vertex_buffers> vertex_buffers{};
    const Buffer* index_buffer = nullptr;   ///< optional
    IndexType index_type = IndexType::None;
};

/// A vertex array object. The referenced buffers must outlive the mesh.
class RIS_API VertexArray {
public:
    explicit VertexArray(const VertexArrayDesc& desc);
    ~VertexArray();
    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;
    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    /// `first` is the first vertex, or the first index when the mesh is indexed.
    /// `instance_count` 0 is treated as 1.
    void draw(Topology topology, uint32_t first, uint32_t count, uint32_t instance_count = 1) const;

private:
    uint32_t vao_ = 0;
    IndexType index_type_ = IndexType::None;
};

// ---- textures -------------------------------------------------------------------------------

enum class Filter : uint32_t {
    Nearest = 0,
    Linear = 1,
    NearestMipmapNearest = 2,
    LinearMipmapNearest = 3,
    NearestMipmapLinear = 4,
    LinearMipmapLinear = 5
};
enum class Wrap : uint32_t { Repeat = 0, Clamp = 1, Mirror = 2 };

struct SamplerDesc {
    Filter min_filter = Filter::LinearMipmapLinear;  ///< mipmap filters need mip levels
    Filter mag_filter = Filter::Linear;              ///< Nearest or Linear
    Wrap wrap_u = Wrap::Clamp;
    Wrap wrap_v = Wrap::Clamp;
};

/// The texture of the default (and only) rendering backend: another name for gles::GlesTexture2D, the OpenGL ES
/// implementation of ATexture2D (ris/rendering/texture/a_texture2D.hpp). `from_rgba8`, `from_image_file`,
/// `from_ktx2_memory`, `set_sampler`, `bind`, `width()`, `height()`, `mip_levels()` and `gl_handle()` are its original
/// API, the ATexture2D getters (`getWidth()`, ...) are available as well.
using Texture = gles::GlesTexture2D;

// ---- render targets -------------------------------------------------------------------------

class RIS_API RenderTarget {
public:
    RenderTarget(int32_t width, int32_t height, bool with_depth_stencil = true);
    ~RenderTarget();
    RenderTarget(RenderTarget&& other) noexcept;
    RenderTarget& operator=(RenderTarget&& other) noexcept;
    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    /// Binds the target and sets the viewport to its size.
    void bind() const;
    /// Binds the window back buffer again and restores the viewport that was active before the first bind().
    static void unbind();

    /// The colour attachment (RGBA8, linear filtering, clamped). Owned by the render target.
    Texture& texture() noexcept { return color_; }
    const Texture& texture() const noexcept { return color_; }

private:
    uint32_t fbo_ = 0;
    uint32_t depth_ = 0;
    Texture color_;
};

} // namespace ris
