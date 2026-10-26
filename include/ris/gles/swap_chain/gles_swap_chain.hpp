#pragma once

#include "ris/rendering/swap_chain/a_swap_chain.hpp"
#include "ris/rendering/swap_chain/swap_chain_descriptor.hpp"
#include "ris/window_manager.hpp"

namespace ris::gles {

/// The OpenGL ES implementation of ASwapChain, like `WebGlSwapChain` of ris-framework-ts. OpenGL ES has no
/// swap chain object: the window's EGL surface is the back buffer, so this is a thin wrapper that presents it
/// (`WindowManager::swapBuffers`) and sets the vsync of the window. The back buffer always has the size of the window
/// (`SwapChainDescriptor::width` / `height` are ignored), its format is Rgba8Unorm. The window must outlive it.
class RIS_API GlesSwapChain : public ASwapChain {
public:
    /// @param window     the window to present to.
    /// @param descriptor the swap chain descriptor. `Immediate` switches vsync off, `Fifo` and `Mailbox` switch it on.
    /// @throws std::invalid_argument for a negative size, std::domain_error for an unknown present mode.
    GlesSwapChain(WindowManager& window, const SwapChainDescriptor& descriptor);
    ~GlesSwapChain() override;

    glm::vec2 getBackBufferSize() const override;
    TextureFormat getTextureFormat() const noexcept override { return TextureFormat::Rgba8Unorm; }
    void resize(int32_t width, int32_t height) override;
    void present() override;

    /// The present mode the swap chain was created with.
    SurfacePresentMode getPresentMode() const noexcept { return presentMode_; }

private:
    WindowManager& window_;
    SurfacePresentMode presentMode_;
};

} // namespace ris::gles
