#pragma once

#include "ris/data.hpp"
#include "ris/rendering/render_pass/load_action.hpp"
#include "ris/rendering/render_pass/store_action.hpp"

namespace ris {

class ASwapChain;   // see ris/rendering/swap_chain/a_swap_chain.hpp
class Texture;      // see ris/graphics.hpp

/// A colour attachment of a render pass, like `RenderPassColorAttachment` of ris-framework-ts. Set exactly one of
/// `swapChain` and `texture`. The `view`, `resolveTexture` and `resolveView` of the TypeScript class are not
/// here, there are no texture views or multisampled textures yet.
struct RenderPassColorAttachment {
    /// The swap chain the render pass draws to. Not owned. A render pass that draws to a swap chain has this
    /// as its only colour attachment.
    ASwapChain* swapChain = nullptr;
    /// The texture the render pass draws to. Not owned, must stay alive for as long as the render pass is used.
    /// Must be set if `swapChain` is not.
    Texture* texture = nullptr;
    /// Whether the contents are kept after the pass, `Store` by default.
    StoreAction storeAction = StoreAction::Store;
    /// What happens to the contents at the start of the pass, `Clear` by default.
    LoadAction loadAction = LoadAction::Clear;
    /// The colour the attachment is cleared to when `loadAction` is `Clear`. Transparent black by default.
    Color clearColor = Color::transparent();
};

} // namespace ris
