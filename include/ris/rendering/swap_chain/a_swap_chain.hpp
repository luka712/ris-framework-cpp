#pragma once

#include <cstdint>

#include "ris/core.hpp"
#include "ris/math.hpp"
#include "ris/rendering/texture/texture_format.hpp"

namespace ris {

/// A swap chain presents the rendered image to the screen, like `ISwapChain` of ris-framework-ts. Abstract and
/// graphics API neutral: every rendering backend provides one implementation (GlesSwapChain for OpenGL ES). Create
/// swap chains with `AGraphicsDevice::createSwapChain`.
///
/// The swap chain is owned by whoever created it (free it with `delete`, before the window and the graphics
/// device).
class RIS_API ASwapChain {
public:
    virtual ~ASwapChain() = default;

    /// The size of the back buffer in pixels.
    virtual glm::vec2 getBackBufferSize() const = 0;

    /// The texture format of the back buffer.
    virtual TextureFormat getTextureFormat() const noexcept = 0;

    /// Sets the back buffer size, to resize it when the window is resized. Backends that always follow the size of
    /// the window (OpenGL ES) only validate the arguments.
    /// @throws std::invalid_argument if `width` or `height` is not greater than zero.
    virtual void resize(int32_t width, int32_t height) = 0;

    /// Presents the back buffer to the screen: call it after rendering is done. Swaps the back and the front buffer.
    /// @throws std::runtime_error if the presentation fails.
    virtual void present() = 0;

    ASwapChain(const ASwapChain&) = delete;
    ASwapChain& operator=(const ASwapChain&) = delete;

protected:
    ASwapChain() = default;
};

} // namespace ris
