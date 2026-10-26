#include "ris/graphics.hpp"

#include <utility>

#include "internal.hpp"
#include "ris/gles/gles.hpp"

using namespace ris::detail;

namespace ris {

namespace {

GLuint g_current_program = 0;

} // namespace

// ---- state ----------------------------------------------------------------------------------

namespace gfx {

void viewport(int32_t x, int32_t y, int32_t width, int32_t height) {

    glViewport(x, y, width, height);
}

void set_scissor(bool enabled, int32_t x, int32_t y, int32_t width, int32_t height) {

    if (enabled) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(x, y, width, height);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
}

void clear(float r, float g, float b, float a, ClearFlags flags) {

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

    using namespace gles::blending_utilities;
    switch (mode) {
        case BlendMode::None:
            disable_blend();
            return;
        case BlendMode::Alpha:
            enable_blend({{BlendOperation::Add, BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha},
                          {BlendOperation::Add, BlendFactor::One, BlendFactor::OneMinusSrcAlpha}});
            break;
        case BlendMode::Premultiplied:
            enable_blend({{BlendOperation::Add, BlendFactor::One, BlendFactor::OneMinusSrcAlpha},
                          {BlendOperation::Add, BlendFactor::One, BlendFactor::OneMinusSrcAlpha}});
            break;
        case BlendMode::Additive:
            enable_blend({{BlendOperation::Add, BlendFactor::SrcAlpha, BlendFactor::One},
                          {BlendOperation::Add, BlendFactor::SrcAlpha, BlendFactor::One}});
            break;
        case BlendMode::Multiply:
            enable_blend({{BlendOperation::Add, BlendFactor::DstColor, BlendFactor::Zero},
                          {BlendOperation::Add, BlendFactor::DstColor, BlendFactor::Zero}});
            break;
    }
}

void set_depth_state(bool test_enabled, bool write_enabled, CompareFunc func) {

    if (test_enabled) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(gles::converter::convert_compare_func(func));
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(write_enabled ? GL_TRUE : GL_FALSE);
}

void set_cull_mode(CullMode mode) {

    if (mode == CullMode::None) {
        gles::culling_utilities::set_culling(CullMode::None);
        return;
    }
    gles::culling_utilities::set_front_face(FrontFace::Ccw);
    gles::culling_utilities::set_culling(mode);
}

std::vector<uint8_t> read_pixels(int32_t x, int32_t y, int32_t width, int32_t height) {

    if (width <= 0 || height <= 0) {
        throw std::invalid_argument(std::string("read_pixels: invalid size ") + std::to_string(width) + "x"
                                    + std::to_string(height));
    }
    std::vector<uint8_t> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 4u);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    return pixels;
}

} // namespace gfx

// ---- Buffer ---------------------------------------------------------------------------------

Buffer::Buffer(BufferType type, BufferUsage usage, const void* data, size_t size) : type_(type), size_(size) {
    id_ = GlesUtilities::buffer().createBuffer(type, data, size, gles::converter::convert_buffer_usage(usage));
}

Buffer::~Buffer() {
    if (id_ != 0 ) {
        glDeleteBuffers(1, &id_);
    }
}

Buffer::Buffer(Buffer&& other) noexcept
    : id_(std::exchange(other.id_, 0)), type_(other.type_), size_(std::exchange(other.size_, 0)) {}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        if (id_ != 0 ) glDeleteBuffers(1, &id_);
        id_ = std::exchange(other.id_, 0);
        type_ = other.type_;
        size_ = std::exchange(other.size_, 0);
    }
    return *this;
}

void Buffer::update(size_t offset, const void* data, size_t size) {

    if (!data || offset + size > size_) {
        throw std::out_of_range(std::string("Buffer::update: range [") + std::to_string(offset) + ", "
                                + std::to_string(offset + size) + ") is outside the buffer (size "
                                + std::to_string(size_) + ")");
    }
    if (size == 0) return;
    GlesUtilities::buffer().updateBuffer(type_, id_, offset, data, size);
}

void Buffer::bind_base(uint32_t binding) const {

    glBindBufferBase(GL_UNIFORM_BUFFER, binding, id_);
}

// ---- Shader ---------------------------------------------------------------------------------

Shader::Shader(const std::string& vertex_source, const std::string& fragment_source) {
    using namespace gles;

    GLuint vs = shader_utilities::create_vertex_shader(vertex_source);
    GLuint fs = 0;
    try {
        fs = shader_utilities::create_fragment_shader(fragment_source);
        program_ = program_utilities::create(vs, fs);
    } catch (...) {
        shader_utilities::delete_shader(vs);
        shader_utilities::delete_shader(fs);
        throw;
    }
    shader_utilities::delete_shader(vs);
    shader_utilities::delete_shader(fs);
}

Shader::~Shader() {
    if (program_ != 0 ) {
        if (g_current_program == program_) g_current_program = 0;
        glDeleteProgram(program_);
    }
}

Shader::Shader(Shader&& other) noexcept : program_(std::exchange(other.program_, 0)) {}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (program_ != 0 ) {
            if (g_current_program == program_) g_current_program = 0;
            glDeleteProgram(program_);
        }
        program_ = std::exchange(other.program_, 0);
    }
    return *this;
}

void Shader::use() const {

    if (g_current_program != program_) {
        glUseProgram(program_);
        g_current_program = program_;
    }
}

int32_t Shader::uniform_location(const char* name) const {

    return glGetUniformLocation(program_, name);
}

int32_t Shader::attrib_location(const char* name) const {

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

    const GLuint index = glGetUniformBlockIndex(program_, block_name);
    if (index == GL_INVALID_INDEX) {
        throw std::invalid_argument(std::string("shader has no uniform block named '") + block_name + "'");
    }
    glUniformBlockBinding(program_, index, binding);
}

// ---- VertexArray -----------------------------------------------------------------------------------

VertexArray::VertexArray(const VertexArrayDesc& desc) : index_type_(desc.index_type) {

    if (desc.attribs.empty() || desc.attribs.size() > 16) {
        throw std::invalid_argument(std::string("VertexArray: attribute count must be 1..16 (got ")
                                    + std::to_string(desc.attribs.size()) + ")");
    }
    if (desc.index_type != IndexType::None && !desc.index_buffer) {
        throw std::invalid_argument("VertexArray: index_type is set but index_buffer is null");
    }
    for (const VertexAttrib& a : desc.attribs) {
        if (a.components < 1 || a.components > 4 || a.buffer_slot >= max_vertex_buffers ||
            !desc.vertex_buffers[a.buffer_slot].buffer) {
            throw std::invalid_argument(std::string("VertexArray: invalid vertex attribute for location ")
                                        + std::to_string(a.location));
        }
    }

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    for (const VertexAttrib& a : desc.attribs) {
        const VertexBufferBinding& binding = desc.vertex_buffers[a.buffer_slot];
        glBindBuffer(GL_ARRAY_BUFFER, binding.buffer->gl_handle());
        gles::vertex_attribute::enable(a, binding.stride);
    }
    if (desc.index_buffer) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, desc.index_buffer->gl_handle());
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);



}

VertexArray::~VertexArray() {
    if (vao_ != 0 ) {
        glDeleteVertexArrays(1, &vao_);
    }
}

VertexArray::VertexArray(VertexArray&& other) noexcept : vao_(std::exchange(other.vao_, 0)), index_type_(other.index_type_) {}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    if (this != &other) {
        if (vao_ != 0 ) glDeleteVertexArrays(1, &vao_);
        vao_ = std::exchange(other.vao_, 0);
        index_type_ = other.index_type_;
    }
    return *this;
}

void VertexArray::draw(Topology topology, uint32_t first, uint32_t count, uint32_t instance_count) const {

    if (count == 0) return;
    if (instance_count == 0) instance_count = 1;
    glBindVertexArray(vao_);
    const GLenum mode = gles::converter::convert_primitive_type(topology);
    if (index_type_ == IndexType::None) {
        glDrawArraysInstanced(mode, static_cast<GLint>(first), static_cast<GLsizei>(count),
                              static_cast<GLsizei>(instance_count));
    } else {
        const bool wide = index_type_ == IndexType::UInt32;
        const uintptr_t byte_offset = static_cast<uintptr_t>(first) * (wide ? 4u : 2u);
        glDrawElementsInstanced(mode, static_cast<GLsizei>(count), gles::converter::convert_index_type(index_type_),
                                reinterpret_cast<const void*>(byte_offset), static_cast<GLsizei>(instance_count));
    }
    glBindVertexArray(0);
}

// ---- RenderTarget ---------------------------------------------------------------------------

RenderTarget::RenderTarget(int32_t width, int32_t height, bool with_depth_stencil) {
    using namespace gles;

    if (width <= 0 || height <= 0) {
        throw std::invalid_argument(std::string("RenderTarget: invalid size ") + std::to_string(width) + "x"
                                    + std::to_string(height));
    }

    // The colour texture is owned by `color_` from here on, so it is released if anything below throws.
    GLuint tex = GlesUtilities::texture().createTexture2D(width, height, static_cast<const void*>(nullptr));
    color_ = gles::GlesTexture2D::adopt(tex, width, height, 1);
    color_.set_sampler(SamplerDesc{Filter::Linear, Filter::Linear, Wrap::Clamp, Wrap::Clamp});

    if (!with_depth_stencil) {
        fbo_ = framebuffer_utilities::create(tex);
        return;
    }
    depth_ = renderbuffer_utilities::create(TextureFormat::Depth24Stencil8, width, height);
    try {
        fbo_ = framebuffer_utilities::create_with_renderbuffer_as_depth_buffer(tex, DepthAttachmentType::DepthStencil,
                                                                               depth_);
    } catch (...) {
        renderbuffer_utilities::delete_renderbuffer(depth_);
        throw;
    }
}

RenderTarget::~RenderTarget() {
        if (depth_) glDeleteRenderbuffers(1, &depth_);
        if (fbo_) glDeleteFramebuffers(1, &fbo_);

}

RenderTarget::RenderTarget(RenderTarget&& other) noexcept
    : fbo_(std::exchange(other.fbo_, 0)), depth_(std::exchange(other.depth_, 0)), color_(std::move(other.color_)) {}

RenderTarget& RenderTarget::operator=(RenderTarget&& other) noexcept {
    if (this != &other) {

            if (depth_) glDeleteRenderbuffers(1, &depth_);
            if (fbo_) glDeleteFramebuffers(1, &fbo_);

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

    GLint current_fbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &current_fbo);
    if (current_fbo == 0) {
        glGetIntegerv(GL_VIEWPORT, t_saved_viewport);
        t_has_saved_viewport = true;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, color_.getWidth(), color_.getHeight());
}

void RenderTarget::unbind() {

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (t_has_saved_viewport) {
        glViewport(t_saved_viewport[0], t_saved_viewport[1], t_saved_viewport[2], t_saved_viewport[3]);
        t_has_saved_viewport = false;
    }
}

} // namespace ris
