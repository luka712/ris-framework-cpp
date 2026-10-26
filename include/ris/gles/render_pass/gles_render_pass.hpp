#pragma once

#include <string>
#include <vector>

#include "ris/data.hpp"
#include <cstdint>
#include "ris/rendering/render_pass/a_render_pass.hpp"
#include "ris/rendering/render_pass/render_pass_descriptor.hpp"

namespace ris::gles {

/// The OpenGL ES 3 implementation of ARenderPass, like `WebGlRenderPass` of ris-framework-ts. A render pass is a
/// framebuffer plus what happens to its attachments:
///  - a swap chain attachment renders to the window's default framebuffer (the descriptor's depth and stencil
///    attachment then describe the window's depth and stencil buffer, which exists when WindowDesc asks for it);
///  - texture attachments render to a framebuffer object with the textures as colour attachments 0, 1, ... and,
///    if the descriptor has a depth and stencil attachment, a 24 bit depth / 8 bit stencil render buffer of the
///    size of the first texture.
/// `beginPass()` binds the framebuffer, sets the viewport to the attachment size, clears the attachments that have
/// `LoadAction::Clear`, and switches the depth test on (less or equal) if there is a depth and stencil attachment,
/// off otherwise. `endPass()` invalidates the attachments with `StoreAction::Discard`. OpenGL ES has no "don't care"
/// load action, `LoadAction::Undefined` keeps the contents like `Load`. Needs the window's GL context for its whole
/// life. Deviations: no multisample resolve, no depth textures, no texture views.
class RIS_API GlesRenderPass : public ARenderPass {
public:
    /// @throws std::invalid_argument for no colour attachment, an attachment with neither or both of swap chain and
    ///         texture, a swap chain together with another colour attachment, or a texture of size 0;
    ///         std::runtime_error if the framebuffer is incomplete; std::logic_error without a window.
    explicit GlesRenderPass(const RenderPassDescriptor& descriptor);
    ~GlesRenderPass() override;

    void beginPass() override;
    void endPass() override;

    /// The framebuffer object name (0 for the window's default framebuffer), for interop with native code.
    uint32_t getGlFramebuffer() const noexcept { return framebuffer_; }

private:
    struct ColorTarget {
        LoadAction loadAction;
        StoreAction storeAction;
        Color clearColor;
    };

    void getTargetSize(int32_t& width, int32_t& height) const;

    std::string label_;
    ASwapChain* swapChain_ = nullptr;
    int32_t width_ = 0;     // texture passes only: the size of the first texture
    int32_t height_ = 0;
    std::vector<ColorTarget> colorTargets_;
    bool hasDepthStencil_ = false;
    RenderPassDepthStencilAttachment depthStencil_;
    uint32_t framebuffer_ = 0;
    uint32_t depthStencilRenderbuffer_ = 0;
    bool active_ = false;
};

} // namespace ris::gles
