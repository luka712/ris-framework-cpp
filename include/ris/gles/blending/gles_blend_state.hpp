#pragma once

#include "ris/gles/gles_types.hpp"
#include "ris/rendering/blending/a_blend_state.hpp"

namespace ris::gles {

/// The OpenGL ES implementation of ABlendState, like `WebGlBlendState` of ris-framework-ts: the blend factors
/// and equations converted to GL enums. Creating it needs no GL context, `apply()` does.
/// Deviation: no colour write mask (not in BlendStateDescriptor).
class RIS_API GlesBlendState : public ABlendState {
public:
    /// @throws std::domain_error for enum values without OpenGL ES mapping.
    explicit GlesBlendState(const BlendStateDescriptor& descriptor = {});

    const BlendStateDescriptor& getDescriptor() const noexcept override { return descriptor_; }

    /// Applies the blend state to the GL context: `glEnable(GL_BLEND)` with `glBlendFuncSeparate` and
    /// `glBlendEquationSeparate`, or `glDisable(GL_BLEND)` when blending is disabled.
    /// @throws std::logic_error without a window.
    void apply() const;

private:
    BlendStateDescriptor descriptor_;
    GlEnum colorSrcFactor_ = 0;
    GlEnum colorDstFactor_ = 0;
    GlEnum colorOperation_ = 0;
    GlEnum alphaSrcFactor_ = 0;
    GlEnum alphaDstFactor_ = 0;
    GlEnum alphaOperation_ = 0;
};

} // namespace ris::gles
