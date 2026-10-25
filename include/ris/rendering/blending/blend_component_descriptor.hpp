#pragma once

#include "ris/rendering/blending/blend_factor.hpp"
#include "ris/rendering/blending/blend_operation.hpp"

namespace ris {

/// The blend equation of one part (colour or alpha) of a blend state, like `BlendComponentDescriptor` of
/// ris-framework-ts: `result = source * srcFactor <operation> destination * dstFactor`. The defaults are the
/// ones of the colour part of a default blend state (classic alpha blending).
struct BlendComponentDescriptor {
    BlendFactor srcFactor = BlendFactor::SrcAlpha;
    BlendFactor dstFactor = BlendFactor::OneMinusSrcAlpha;
    BlendOperation operation = BlendOperation::Add;

    constexpr bool operator==(const BlendComponentDescriptor&) const = default;
};

} // namespace ris
