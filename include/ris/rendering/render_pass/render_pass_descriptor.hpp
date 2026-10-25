#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ris/rendering/render_pass/render_pass_color_attachment.hpp"
#include "ris/rendering/render_pass/render_pass_depth_stencil_attachment.hpp"

namespace ris {

/// Describes a render pass, like `RenderPassDescriptor` of ris-framework-ts.
struct RenderPassDescriptor {
    /// The colour attachments of the render pass, at least one.
    std::vector<RenderPassColorAttachment> colorAttachments;
    /// The depth and stencil buffer. Without it the pass has no depth test.
    std::optional<RenderPassDepthStencilAttachment> depthStencilAttachment;
    /// A label for graphics debuggers. Does not affect the rendering.
    std::string label;
};

} // namespace ris
