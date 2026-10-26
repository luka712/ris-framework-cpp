#include "ris/gles/render_pass/gles_render_pass.hpp"

#include <span>
#include <stdexcept>
#include <string>

#include "internal.hpp"
#include "ris/gles/utilities/framebuffer_utilities.hpp"
#include "ris/gles/utilities/renderbuffer_utilities.hpp"
#include "ris/graphics.hpp"
#include "ris/rendering/swap_chain/a_swap_chain.hpp"

using namespace ris::detail;

namespace ris::gles {

GlesRenderPass::GlesRenderPass(const RenderPassDescriptor& descriptor) : label_(descriptor.label) {
    require_context();
    if (descriptor.colorAttachments.empty()) {
        throw std::invalid_argument("GlesRenderPass: a render pass needs at least one color attachment");
    }

    std::vector<uint32_t> textures;
    for (const RenderPassColorAttachment& attachment : descriptor.colorAttachments) {
        if ((attachment.swapChain != nullptr) == (attachment.texture != nullptr)) {
            throw std::invalid_argument("GlesRenderPass: a color attachment needs exactly one of swapChain and texture");
        }
        if (attachment.swapChain != nullptr) {
            if (descriptor.colorAttachments.size() != 1) {
                throw std::invalid_argument("GlesRenderPass: a swap chain must be the only color attachment (it is the default framebuffer)");
            }
            swapChain_ = attachment.swapChain;
        } else {
            if (attachment.texture->getWidth() <= 0 || attachment.texture->getHeight() <= 0 || attachment.texture->getHandle() == 0) {
                throw std::invalid_argument("GlesRenderPass: a color attachment texture is empty (moved from?)");
            }
            if (textures.empty()) {
                width_ = attachment.texture->getWidth();
                height_ = attachment.texture->getHeight();
            }
            textures.push_back(static_cast<uint32_t>(attachment.texture->getHandle()));
        }
        colorTargets_.push_back({attachment.loadAction, attachment.storeAction, attachment.clearColor});
    }

    if (descriptor.depthStencilAttachment.has_value()) {
        hasDepthStencil_ = true;
        depthStencil_ = *descriptor.depthStencilAttachment;
    }

    if (swapChain_ == nullptr) {
        framebuffer_ = framebuffer_utilities::create(std::span<const uint32_t>(textures), label_);
        if (hasDepthStencil_) {
            try {
                depthStencilRenderbuffer_ = renderbuffer_utilities::create(TextureFormat::Depth24Stencil8, width_, height_, label_);
                framebuffer_utilities::attach_depth_stencil_renderbuffer(framebuffer_, depthStencilRenderbuffer_, TextureFormat::Depth24Stencil8);
            } catch (...) {
                renderbuffer_utilities::delete_renderbuffer(depthStencilRenderbuffer_);
                framebuffer_utilities::delete_framebuffer(framebuffer_);
                throw;
            }
        }
    }
}

GlesRenderPass::~GlesRenderPass() {
    framebuffer_utilities::delete_framebuffer(framebuffer_);
    renderbuffer_utilities::delete_renderbuffer(depthStencilRenderbuffer_);
}

void GlesRenderPass::getTargetSize(int32_t& width, int32_t& height) const {
    if (swapChain_ != nullptr) {
        const glm::vec2 size = swapChain_->getBackBufferSize();
        width = static_cast<int32_t>(size.x);
        height = static_cast<int32_t>(size.y);
    } else {
        width = width_;
        height = height_;
    }
}

void GlesRenderPass::beginPass() {
    require_context();
    if (active_) throw std::invalid_argument("GlesRenderPass::beginPass: the pass was already begun, call endPass() first");

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    int32_t width = 0;
    int32_t height = 0;
    getTargetSize(width, height);
    glViewport(0, 0, width, height);

    // The write masks also apply to clears, make sure everything is written.
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    for (size_t i = 0; i < colorTargets_.size(); ++i) {
        if (colorTargets_[i].loadAction != LoadAction::Clear) continue;
        const Color& c = colorTargets_[i].clearColor;
        const GLfloat rgba[4] = {c.r, c.g, c.b, c.a};
        glClearBufferfv(GL_COLOR, static_cast<GLint>(i), rgba);
    }

    if (hasDepthStencil_) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_TRUE);
        glStencilMask(0xFF);
        if (depthStencil_.depthLoadAction == LoadAction::Clear) {
            glClearBufferfv(GL_DEPTH, 0, &depthStencil_.depthClearValue);
        }
        if (depthStencil_.stencilLoadAction == LoadAction::Clear) {
            const GLint stencil = static_cast<GLint>(depthStencil_.stencilClearValue);
            glClearBufferiv(GL_STENCIL, 0, &stencil);
        }
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    check_gl("GlesRenderPass::beginPass");
    active_ = true;
}

void GlesRenderPass::endPass() {
    require_context();
    if (!active_) throw std::invalid_argument("GlesRenderPass::endPass: the pass was not begun, call beginPass() first");
    active_ = false;

    // The default framebuffer has GL_COLOR / GL_DEPTH / GL_STENCIL, a framebuffer object the *_ATTACHMENT points.
    const bool defaultFramebuffer = framebuffer_ == 0;
    std::vector<GLenum> discarded;
    for (size_t i = 0; i < colorTargets_.size(); ++i) {
        if (colorTargets_[i].storeAction == StoreAction::Discard) {
            discarded.push_back(defaultFramebuffer ? GL_COLOR : GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));
        }
    }
    if (hasDepthStencil_) {
        if (depthStencil_.depthStoreAction == StoreAction::Discard) {
            discarded.push_back(defaultFramebuffer ? GL_DEPTH : GL_DEPTH_ATTACHMENT);
        }
        if (depthStencil_.stencilStoreAction == StoreAction::Discard) {
            discarded.push_back(defaultFramebuffer ? GL_STENCIL : GL_STENCIL_ATTACHMENT);
        }
    }
    if (!discarded.empty()) {
        glInvalidateFramebuffer(GL_FRAMEBUFFER, static_cast<GLsizei>(discarded.size()), discarded.data());
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    check_gl("GlesRenderPass::endPass");
}

} // namespace ris::gles
