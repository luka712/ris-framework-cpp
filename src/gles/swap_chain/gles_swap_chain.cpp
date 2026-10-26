#include "ris/gles/swap_chain/gles_swap_chain.hpp"

#include <stdexcept>
#include <string>

namespace ris::gles {

GlesSwapChain::GlesSwapChain(WindowManager& window, const SwapChainDescriptor& descriptor)
    : window_(window), presentMode_(descriptor.desiredPresentationMode) {
    if (descriptor.width < 0 || descriptor.height < 0) {
        throw std::invalid_argument("GlesSwapChain: the size must not be negative (got " + std::to_string(descriptor.width) + "x" + std::to_string(descriptor.height) + ")");
    }
    switch (presentMode_) {
        case SurfacePresentMode::Immediate: window_.setVsync(false); return;
        case SurfacePresentMode::Fifo:
        case SurfacePresentMode::Mailbox: window_.setVsync(true); return;
    }
    throw std::domain_error("GlesSwapChain: unknown present mode " + std::to_string(static_cast<uint32_t>(presentMode_)));
}

GlesSwapChain::~GlesSwapChain() = default;

glm::vec2 GlesSwapChain::getBackBufferSize() const {
    return window_.getWindowBounds();
}

void GlesSwapChain::resize(int32_t width, int32_t height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("GlesSwapChain::resize: the size must be positive (got " + std::to_string(width) + "x" + std::to_string(height) + ")");
    }
    // The back buffer is the window's surface: it follows the size of the window by itself.
}

void GlesSwapChain::present() {
    window_.swapBuffers();
}

} // namespace ris::gles
