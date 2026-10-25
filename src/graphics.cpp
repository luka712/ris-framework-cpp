#include "ris/graphics.hpp"

#include <utility>

#include "internal.hpp"

using namespace ris::detail;

namespace ris {

namespace {

GLenum compare_func(CompareFunc f) {
    switch (f) {
        case CompareFunc::Never: return GL_NEVER;
        case CompareFunc::Less: return GL_LESS;
        case CompareFunc::Equal: return GL_EQUAL;
        case CompareFunc::LessEqual: return GL_LEQUAL;
        case CompareFunc::Greater: return GL_GREATER;
        case CompareFunc::NotEqual: return GL_NOTEQUAL;
        case CompareFunc::GreaterEqual: return GL_GEQUAL;
        case CompareFunc::Always: return GL_ALWAYS;
    }
    return GL_LESS;
}

GLenum buffer_target(BufferType t) {
    switch (t) {
        case BufferType::Vertex: return GL_ARRAY_BUFFER;
        case BufferType::Index: return GL_ELEMENT_ARRAY_BUFFER;
        case BufferType::Uniform: return GL_UNIFORM_BUFFER;
    }
    return GL_ARRAY_BUFFER;
}

GLenum buffer_usage(BufferUsage u) {
    switch (u) {
        case BufferUsage::Static: return GL_STATIC_DRAW;
        case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
        case BufferUsage::Stream: return GL_STREAM_DRAW;
    }
    return GL_STATIC_DRAW;
}

GLuint g_current_program = 0;

} // namespace

// ---- state ----------------------------------------------------------------------------------

namespace gfx {

void viewport(int32_t x, int32_t y, int32_t width, int32_t height) {
    require_context();
    glViewport(x, y, width, height);
}

void set_scissor(bool enabled, int32_t x, int32_t y, int32_t width, int32_t height) {
    require_context();
    if (enabled) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(x, y, width, height);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
}

void clear(float r, float g, float b, float a, ClearFlags flags) {
    require_context();
    GLbitfield mask = 0;
    const uint32_t f = static_cast<uint32_t>(flags);
    if (f & static_cast<uint32_t>(ClearFlags::Color)) {
        glClearColor(r, g, b, a);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        mask |= GL_COLOR_BUFFER_BIT;
    }
    if (f & static_cast<uint32_t>(ClearFlags::Depth)) {
        glDepthMask(GL_TRUE);   // clearing is subject to the depth mask
        glClearDepthf(1.0f);
        mask |= GL_DEPTH_BUFFER_BIT;
    }
    if (f & static_cast<uint32_t>(ClearFlags::Stencil)) {
        glClearStencil(0);
        mask |= GL_STENCIL_BUFFER_BIT;
    }
    glClear(mask);
}

void set_blend_mode(BlendMode mode) {
    require_context();
    switch (mode) {
        case BlendMode::None:
            glDisable(GL_BLEND);
            return;
        case BlendMode::Alpha:
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case BlendMode::Premultiplied:
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case BlendMode::Additive:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case BlendMode::Multiply:
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            break;
    }
    glBlendEquation(GL_FUNC_ADD);
    glEnable(GL_BLEND);
}

void set_depth_state(bool test_enabled, bool write_enabled, CompareFunc func) {
    require_context();
    if (test_enabled) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(compare_func(func));
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(write_enabled ? GL_TRUE : GL_FALSE);
}

void set_cull_mode(CullMode mode) {
    require_context();
    if (mode == CullMode::None) {
        glDisable(GL_CULL_FACE);
        return;
    }
    glFrontFace(GL_CCW);
    glCullFace(mode == CullMode::Back ? GL_BACK : GL_FRONT);
    glEnable(GL_CULL_FACE);
}

std::vector<uint8_t> read_pixels(int32_t x, int32_t y, int32_t width, int32_t height) {
    require_context();
    if (width <= 0 || height <= 0) {
        throw_error(Result::InvalidArgument, "read_pixels: invalid size %dx%d", width, height);
    }
    std::vector<uint8_t> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 4u);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    check_gl("glReadPixels");
    return pixels;
}

} // namespace gfx

// ---- Buffer ---------------------------------------------------------------------------------

Buffer::Buffer(BufferType type, BufferUsage usage, const void* data, size_t size) : type_(type), size_(size) {
    require_context();
    if (size == 0) {
        throw_error(Result::InvalidArgument, "Buffer: size must be greater than zero");
    }
    glGenBuffers(1, &id_);
    // GL_COPY_WRITE_BUFFER keeps the VAO's element array binding untouched.
    const GLenum target = type == BufferType::Index ? GL_COPY_WRITE_BUFFER : buffer_target(type);
    glBindBuffer(target, id_);
    glBufferData(target, static_cast<GLsizeiptr>(size), data, buffer_usage(usage));
    glBindBuffer(target, 0);
    try {
        check_gl("Buffer::Buffer");
    } catch (...) {
        glDeleteBuffers(1, &id_);
        id_ = 0;
        throw;
    }
}

Buffer::~Buffer() {
    if (id_ != 0 && has_context()) {
        glDeleteBuffers(1, &id_);
    }
}

Buffer::Buffer(Buffer&& other) noexcept
    : id_(std::exchange(other.id_, 0)), type_(other.type_), size_(std::exchange(other.size_, 0)) {}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        if (id_ != 0 && has_context()) glDeleteBuffers(1, &id_);
        id_ = std::exchange(other.id_, 0);
        type_ = other.type_;
        size_ = std::exchange(other.size_, 0);
    }
    return *this;
}

void Buffer::update(size_t offset, const void* data, size_t size) {
    require_context();
    if (!data || offset + size > size_) {
        throw_error(Result::InvalidArgument, "Buffer::update: range [%zu, %zu) is outside the buffer (size %zu)", offset,
                    offset + size, size_);
    }
    const GLenum target = type_ == BufferType::Index ? GL_COPY_WRITE_BUFFER : buffer_target(type_);
    glBindBuffer(target, id_);
    glBufferSubData(target, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
    glBindBuffer(target, 0);
    check_gl("Buffer::update");
}

void Buffer::bind_base(uint32_t binding) const {
    require_context();
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, id_);
}

// ---- Shader ---------------------------------------------------------------------------------

namespace {

GLuint compile_stage(GLenum type, const std::string& source, const char* stage_name) {
    const GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::string log_text(static_cast<size_t>(len > 0 ? len : 1), '\0');
        glGetShaderInfoLog(shader, len, nullptr, log_text.data());
        glDeleteShader(shader);
        throw_error(Result::Compile, "%s shader compile error:\n%s", stage_name, log_text.c_str());
    }
    return shader;
}

} // namespace

Shader::Shader(const std::string& vertex_source, const std::string& fragment_source) {
    require_context();
    const GLuint vs = compile_stage(GL_VERTEX_SHADER, vertex_source, "vertex");
    GLuint fs = 0;
    try {
        fs = compile_stage(GL_FRAGMENT_SHADER, fragment_source, "fragment");
    } catch (...) {
        glDeleteShader(vs);
        throw;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);
    glDetachShader(program_, vs);
    glDetachShader(program_, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &len);
        std::string log_text(static_cast<size_t>(len > 0 ? len : 1), '\0');
        glGetProgramInfoLog(program_, len, nullptr, log_text.data());
        glDeleteProgram(program_);
        program_ = 0;
        throw_error(Result::Compile, "shader link error:\n%s", log_text.c_str());
    }
}

Shader::~Shader() {
    if (program_ != 0 && has_context()) {
        if (g_current_program == program_) g_current_program = 0;
        glDeleteProgram(program_);
    }
}

Shader::Shader(Shader&& other) noexcept : program_(std::exchange(other.program_, 0)) {}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (program_ != 0 && has_context()) {
            if (g_current_program == program_) g_current_program = 0;
            glDeleteProgram(program_);
        }
        program_ = std::exchange(other.program_, 0);
    }
    return *this;
}

void Shader::use() const {
    require_context();
    if (g_current_program != program_) {
        glUseProgram(program_);
        g_current_program = program_;
    }
}

int32_t Shader::uniform_location(const char* name) const {
    require_context();
    return glGetUniformLocation(program_, name);
}

int32_t Shader::attrib_location(const char* name) const {
    require_context();
    return glGetAttribLocation(program_, name);
}

void Shader::set_int(int32_t loc, int32_t v) const {
    if (loc < 0) return;
    use();
    glUniform1i(loc, v);
}
void Shader::set_float(int32_t loc, float v) const {
    if (loc < 0) return;
    use();
    glUniform1f(loc, v);
}
void Shader::set_vec2(int32_t loc, float x, float y) const {
    if (loc < 0) return;
    use();
    glUniform2f(loc, x, y);
}
void Shader::set_vec3(int32_t loc, float x, float y, float z) const {
    if (loc < 0) return;
    use();
    glUniform3f(loc, x, y, z);
}
void Shader::set_vec4(int32_t loc, float x, float y, float z, float w) const {
    if (loc < 0) return;
    use();
    glUniform4f(loc, x, y, z, w);
}
void Shader::set_mat4(int32_t loc, const float* m) const {
    if (loc < 0 || !m) return;
    use();
    glUniformMatrix4fv(loc, 1, GL_FALSE, m);
}

void Shader::bind_uniform_block(const char* block_name, uint32_t binding) const {
    require_context();
    const GLuint index = glGetUniformBlockIndex(program_, block_name);
    if (index == GL_INVALID_INDEX) {
        throw_error(Result::InvalidArgument, "shader has no uniform block named '%s'", block_name);
    }
    glUniformBlockBinding(program_, index, binding);
}

// ---- VertexArray -----------------------------------------------------------------------------------

namespace {

GLenum attrib_gl_type(AttribType t) {
    switch (t) {
        case AttribType::Float: return GL_FLOAT;
        case AttribType::UByte: return GL_UNSIGNED_BYTE;
        case AttribType::Byte: return GL_BYTE;
        case AttribType::UShort: return GL_UNSIGNED_SHORT;
        case AttribType::Short: return GL_SHORT;
        case AttribType::UInt: return GL_UNSIGNED_INT;
        case AttribType::Int: return GL_INT;
    }
    return GL_FLOAT;
}

GLenum topology_gl(Topology t) {
    switch (t) {
        case Topology::Triangles: return GL_TRIANGLES;
        case Topology::TriangleStrip: return GL_TRIANGLE_STRIP;
        case Topology::TriangleFan: return GL_TRIANGLE_FAN;
        case Topology::Lines: return GL_LINES;
        case Topology::LineStrip: return GL_LINE_STRIP;
        case Topology::Points: return GL_POINTS;
    }
    return GL_TRIANGLES;
}

} // namespace

VertexArray::VertexArray(const VertexArrayDesc& desc) : index_type_(desc.index_type) {
    require_context();
    if (desc.attribs.empty() || desc.attribs.size() > 16) {
        throw_error(Result::InvalidArgument, "VertexArray: attribute count must be 1..16 (got %zu)", desc.attribs.size());
    }
    if (desc.index_type != IndexType::None && !desc.index_buffer) {
        throw_error(Result::InvalidArgument, "VertexArray: index_type is set but index_buffer is null");
    }
    for (const VertexAttrib& a : desc.attribs) {
        if (a.components < 1 || a.components > 4 || a.buffer_slot >= max_vertex_buffers ||
            !desc.vertex_buffers[a.buffer_slot].buffer) {
            throw_error(Result::InvalidArgument, "VertexArray: invalid vertex attribute for location %u", a.location);
        }
    }

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    for (const VertexAttrib& a : desc.attribs) {
        const VertexBufferBinding& binding = desc.vertex_buffers[a.buffer_slot];
        glBindBuffer(GL_ARRAY_BUFFER, binding.buffer->gl_handle());
        const void* offset = reinterpret_cast<const void*>(static_cast<uintptr_t>(a.offset));
        if (a.as_integer && a.type != AttribType::Float) {
            glVertexAttribIPointer(a.location, static_cast<GLint>(a.components), attrib_gl_type(a.type),
                                   static_cast<GLsizei>(binding.stride), offset);
        } else {
            glVertexAttribPointer(a.location, static_cast<GLint>(a.components), attrib_gl_type(a.type),
                                  a.normalized ? GL_TRUE : GL_FALSE, static_cast<GLsizei>(binding.stride), offset);
        }
        glEnableVertexAttribArray(a.location);
        glVertexAttribDivisor(a.location, a.divisor);
    }
    if (desc.index_buffer) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, desc.index_buffer->gl_handle());
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    try {
        check_gl("VertexArray::VertexArray");
    } catch (...) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
        throw;
    }
}

VertexArray::~VertexArray() {
    if (vao_ != 0 && has_context()) {
        glDeleteVertexArrays(1, &vao_);
    }
}

VertexArray::VertexArray(VertexArray&& other) noexcept : vao_(std::exchange(other.vao_, 0)), index_type_(other.index_type_) {}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    if (this != &other) {
        if (vao_ != 0 && has_context()) glDeleteVertexArrays(1, &vao_);
        vao_ = std::exchange(other.vao_, 0);
        index_type_ = other.index_type_;
    }
    return *this;
}

void VertexArray::draw(Topology topology, uint32_t first, uint32_t count, uint32_t instance_count) const {
    require_context();
    if (count == 0) return;
    if (instance_count == 0) instance_count = 1;
    glBindVertexArray(vao_);
    const GLenum mode = topology_gl(topology);
    if (index_type_ == IndexType::None) {
        glDrawArraysInstanced(mode, static_cast<GLint>(first), static_cast<GLsizei>(count),
                              static_cast<GLsizei>(instance_count));
    } else {
        const bool wide = index_type_ == IndexType::UInt32;
        const uintptr_t byte_offset = static_cast<uintptr_t>(first) * (wide ? 4u : 2u);
        glDrawElementsInstanced(mode, static_cast<GLsizei>(count), wide ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
                                reinterpret_cast<const void*>(byte_offset), static_cast<GLsizei>(instance_count));
    }
    glBindVertexArray(0);
}

// ---- RenderTarget ---------------------------------------------------------------------------

RenderTarget::RenderTarget(int32_t width, int32_t height, bool with_depth_stencil) {
    require_context();
    if (width <= 0 || height <= 0) {
        throw_error(Result::InvalidArgument, "RenderTarget: invalid size %dx%d", width, height);
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    color_ = Texture::adopt(tex, width, height, 1);

    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    if (with_depth_stencil) {
        glGenRenderbuffers(1, &depth_);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        if (depth_) glDeleteRenderbuffers(1, &depth_);
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = depth_ = 0;
        throw_error(Result::Graphics, "RenderTarget: framebuffer incomplete (0x%04X)", static_cast<unsigned>(status));
    }
}

RenderTarget::~RenderTarget() {
    if (has_context()) {
        if (depth_) glDeleteRenderbuffers(1, &depth_);
        if (fbo_) glDeleteFramebuffers(1, &fbo_);
    }
}

RenderTarget::RenderTarget(RenderTarget&& other) noexcept
    : fbo_(std::exchange(other.fbo_, 0)), depth_(std::exchange(other.depth_, 0)), color_(std::move(other.color_)) {}

RenderTarget& RenderTarget::operator=(RenderTarget&& other) noexcept {
    if (this != &other) {
        if (has_context()) {
            if (depth_) glDeleteRenderbuffers(1, &depth_);
            if (fbo_) glDeleteFramebuffers(1, &fbo_);
        }
        fbo_ = std::exchange(other.fbo_, 0);
        depth_ = std::exchange(other.depth_, 0);
        color_ = std::move(other.color_);
    }
    return *this;
}

namespace {
// Viewport of the window back buffer, saved when the first render target is bound.
thread_local GLint t_saved_viewport[4] = {0, 0, 0, 0};
thread_local bool t_has_saved_viewport = false;
} // namespace

void RenderTarget::bind() const {
    require_context();
    GLint current_fbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &current_fbo);
    if (current_fbo == 0) {
        glGetIntegerv(GL_VIEWPORT, t_saved_viewport);
        t_has_saved_viewport = true;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, color_.width(), color_.height());
}

void RenderTarget::unbind() {
    require_context();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (t_has_saved_viewport) {
        glViewport(t_saved_viewport[0], t_saved_viewport[1], t_saved_viewport[2], t_saved_viewport[3]);
        t_has_saved_viewport = false;
    }
}

} // namespace ris
