#pragma once

#include "ris/core.hpp"

namespace ris {

/// A render pass, like `IRenderPass` of ris-framework-ts: a set of attachments that are drawn to between
/// `beginPass()` and `endPass()`. Abstract and graphics API neutral: every rendering backend provides one
/// implementation (GlesRenderPass for OpenGL ES). Create render passes with `AGraphicsDevice::createRenderPass`.
///
/// The render pass is owned by whoever created it (free it with `delete`, before the graphics device). It does
/// not own the textures and the swap chain it draws to.
class RIS_API ARenderPass {
public:
    virtual ~ARenderPass() = default;

    /// Begins the render pass: makes its attachments the target of the drawing and loads them (clears the ones with
    /// `LoadAction::Clear`).
    /// @throws std::invalid_argument if the pass was already begun.
    virtual void beginPass() = 0;

    /// Ends the render pass: attachments with `StoreAction::Discard` are discarded.
    /// @throws std::invalid_argument if the pass was not begun.
    virtual void endPass() = 0;

    ARenderPass(const ARenderPass&) = delete;
    ARenderPass& operator=(const ARenderPass&) = delete;

protected:
    ARenderPass() = default;
};

} // namespace ris
