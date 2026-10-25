#pragma once

#include "ris/core.hpp"
#include "ris/rendering/blending/blend_factor.hpp"
#include "ris/rendering/blending/blend_operation.hpp"

/// Blending state of the current context, like `WebGLBlendingUtilities` of ris-framework-ts. They need the
/// window's GL context and throw std::logic_error without one.
namespace ris::gles::blending_utilities {

/// Blend equation and factors for the colour or the alpha channel.
struct BlendComponent {
    BlendOperation operation = BlendOperation::Add;
    BlendFactor src_factor = BlendFactor::SrcAlpha;
    BlendFactor dst_factor = BlendFactor::OneMinusSrcAlpha;
};

/// The parameters of enable_blend(). The defaults are those of ris-framework-ts: classic alpha blending with
/// `FUNC_ADD`, `SRC_ALPHA` and `ONE_MINUS_SRC_ALPHA` for colour and alpha.
struct BlendParams {
    BlendComponent color;
    BlendComponent alpha;
};

/// Enables blending: `glEnable(GL_BLEND)`, `glBlendFuncSeparate`, `glBlendEquationSeparate`. `enableBlend`
/// of ris-framework-ts (which takes six nullable numbers; the struct has the same defaults).
RIS_API void enable_blend(const BlendParams& params = {});

/// Disables blending (`glDisable(GL_BLEND)`). Factors and equations stay as they are.
RIS_API void disable_blend();

} // namespace ris::gles::blending_utilities
