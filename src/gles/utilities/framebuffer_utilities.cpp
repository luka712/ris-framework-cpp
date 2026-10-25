#include "ris/gles/utilities/framebuffer_utilities.hpp"

#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include "internal.hpp"
#include "ris/gles/utilities/debug_label_utilities.hpp"

using namespace ris::detail;

namespace ris::gles::framebuffer_utilities {

namespace {

GLenum attachment_point(DepthAttachmentType type) {
    switch (type) {
        case DepthAttachmentType::Depth: return GL_DEPTH_ATTACHMENT;
        case DepthAttachmentType::DepthStencil: return GL_DEPTH_STENCIL_ATTACHMENT;
        case DepthAttachmentType::Stencil: return GL_STENCIL_ATTACHMENT;
    }
    throw std::invalid_argument(std::string("invalid depth attachment type ")
                                + std::to_string(static_cast<unsigned>(type)));
}

/// Creates and binds a framebuffer. Restores the previous binding and deletes the framebuffer unless
/// commit() was called.
class FramebufferScope {
public:
    FramebufferScope() {
        GLint previous = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous);
        previous_ = static_cast<GLuint>(previous);
        glGenFramebuffers(1, &id_);
        glBindFramebuffer(GL_FRAMEBUFFER, id_);
    }
    FramebufferScope(const FramebufferScope&) = delete;
    FramebufferScope& operator=(const FramebufferScope&) = delete;
    ~FramebufferScope() {
        glBindFramebuffer(GL_FRAMEBUFFER, previous_);
        if (!committed_) glDeleteFramebuffers(1, &id_);
    }

    GLuint id() const { return id_; }

    /// Throws if the framebuffer is not complete.
    void require_complete() const {
        const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            char hex[16];
            std::snprintf(hex, sizeof hex, "%04X", static_cast<unsigned>(status));
            throw std::runtime_error(std::string("Framebuffer is not complete (status 0x") + hex + ")");
        }
    }

    GLuint commit(std::string_view label) {
        committed_ = true;
        debug_label_utilities::set_label(debug_label_utilities::ObjectKind::Framebuffer, id_, label);
        return id_;
    }

private:
    GLuint id_ = 0;
    GLuint previous_ = 0;
    bool committed_ = false;
};

/// Binds a framebuffer for the scope and restores the previous binding.
class BindScope {
public:
    explicit BindScope(GLuint framebuffer) {
        GLint previous = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous);
        previous_ = static_cast<GLuint>(previous);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    }
    BindScope(const BindScope&) = delete;
    BindScope& operator=(const BindScope&) = delete;
    ~BindScope() { glBindFramebuffer(GL_FRAMEBUFFER, previous_); }

private:
    GLuint previous_ = 0;
};

} // namespace

GlUint create(std::span<const GlUint> color_textures, std::string_view label) {
    require_context();
    if (color_textures.empty()) {
        throw std::invalid_argument("framebuffer_utilities::create: no colour textures");
    }
    for (const GlUint texture : color_textures) {
        if (texture == 0) {
            throw std::invalid_argument("Cannot bind texture to the frame buffer. Texture id not specified");
        }
    }

    FramebufferScope fb;
    std::vector<GLenum> draw_buffers;
    for (size_t i = 0; i < color_textures.size(); ++i) {
        const GLenum attachment = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, color_textures[i], 0);
        draw_buffers.push_back(attachment);
    }
    if (draw_buffers.size() > 1) glDrawBuffers(static_cast<GLsizei>(draw_buffers.size()), draw_buffers.data());
    fb.require_complete();
    return fb.commit(label);
}

GlUint create(GlUint color_texture, std::string_view label) {
    return create(std::span<const GlUint>(&color_texture, 1), label);
}

GlUint create_with_renderbuffer_as_depth_buffer(GlUint color_texture, DepthAttachmentType depth_type,
                                                GlUint render_buffer, std::string_view label) {
    require_context();
    const GLenum attachment = attachment_point(depth_type);
    FramebufferScope fb;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, render_buffer);
    fb.require_complete();
    return fb.commit(label);
}

GlUint create_with_texture_as_depth_buffer(GlUint color_texture, DepthAttachmentType depth_type, GlUint depth_texture,
                                           std::string_view label) {
    require_context();
    const GLenum attachment = attachment_point(depth_type);
    FramebufferScope fb;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, depth_texture, 0);
    fb.require_complete();
    return fb.commit(label);
}

DepthAttachmentType depth_attachment_type_for(TextureFormat format) {
    switch (format) {
        case TextureFormat::Depth32Float: return DepthAttachmentType::Depth;
        case TextureFormat::Depth24Stencil8: return DepthAttachmentType::DepthStencil;
        default: break;
    }
    throw std::domain_error("Unsupported depth/stencil texture format");
}

void attach_depth_stencil_renderbuffer(GlUint framebuffer, GlUint render_buffer, TextureFormat format) {
    require_context();
    const GLenum attachment = attachment_point(depth_attachment_type_for(format));
    BindScope bound(framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, render_buffer);
}

void attach_depth_stencil_texture(GlUint framebuffer, GlUint depth_texture, TextureFormat format) {
    require_context();
    const GLenum attachment = attachment_point(depth_attachment_type_for(format));
    BindScope bound(framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, depth_texture, 0);
}

void bind(GlUint framebuffer) {
    require_context();
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
}

void delete_framebuffer(GlUint& framebuffer) noexcept {
    if (framebuffer != 0 && has_context()) {
        const GLuint id = framebuffer;
        glDeleteFramebuffers(1, &id);
    }
    framebuffer = 0;
}

} // namespace ris::gles::framebuffer_utilities
