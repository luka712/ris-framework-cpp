#pragma once

#include "ris/rendering/blending/blend_component_descriptor.hpp"

namespace ris {

/// Describes the blend state of a render pipeline, like `BlendStateDescriptor` of ris-framework-ts. The
/// default is alpha blending: colour `SrcAlpha, OneMinusSrcAlpha, Add` and alpha `One, OneMinusSrcAlpha, Add`.
struct BlendStateDescriptor {
    /// If false the source colour is written without blending.
    bool blendingEnabled = true;
    /// The colour equation.
    BlendComponentDescriptor color{BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha, BlendOperation::Add};
    /// The alpha equation.
    BlendComponentDescriptor alpha{BlendFactor::One, BlendFactor::OneMinusSrcAlpha, BlendOperation::Add};

    constexpr bool operator==(const BlendStateDescriptor&) const = default;
};

} // namespace ris
