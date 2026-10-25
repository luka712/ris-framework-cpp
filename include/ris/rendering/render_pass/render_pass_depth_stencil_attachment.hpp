#pragma once

#include <cstdint>

#include "ris/rendering/render_pass/load_action.hpp"
#include "ris/rendering/render_pass/store_action.hpp"

namespace ris {

/// Describes the depth and stencil buffer of a render pass, like `RenderPassDepthStencilAttachment` of
/// ris-framework-ts. How they are loaded, cleared and stored. The `view` and `texture` of the TypeScript class are
/// not here: the render pass has its own depth and stencil buffer (a 24 bit depth, 8 bit stencil render buffer
/// for offscreen passes, the one of the window for a swap chain), there are no depth textures yet.
struct RenderPassDepthStencilAttachment {
    /// What happens to the depth buffer at the start of the pass, `Clear` by default.
    LoadAction depthLoadAction = LoadAction::Clear;
    /// What happens to the depth buffer at the end of the pass, `Discard` by default (it cannot be read later).
    StoreAction depthStoreAction = StoreAction::Discard;
    /// The depth the buffer is cleared to when `depthLoadAction` is `Clear`, typically 1 (the far plane).
    float depthClearValue = 1.0f;
    /// What happens to the stencil buffer at the start of the pass, `Clear` by default.
    LoadAction stencilLoadAction = LoadAction::Clear;
    /// What happens to the stencil buffer at the end of the pass, `Discard` by default.
    StoreAction stencilStoreAction = StoreAction::Discard;
    /// The value the stencil buffer is cleared to when `stencilLoadAction` is `Clear`.
    uint32_t stencilClearValue = 0;
};

} // namespace ris
