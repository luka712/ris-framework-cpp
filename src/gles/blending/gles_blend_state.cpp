#include "ris/gles/blending/gles_blend_state.hpp"

#include "internal.hpp"
#include "ris/gles/utilities/gles_converter.hpp"

using namespace ris::detail;

namespace ris::gles {

GlesBlendState::GlesBlendState(const BlendStateDescriptor& descriptor) : descriptor_(descriptor) {
    colorSrcFactor_ = converter::convert_blend_factor(descriptor.color.srcFactor);
    colorDstFactor_ = converter::convert_blend_factor(descriptor.color.dstFactor);
    colorOperation_ = converter::convert_blend_operation(descriptor.color.operation);
    alphaSrcFactor_ = converter::convert_blend_factor(descriptor.alpha.srcFactor);
    alphaDstFactor_ = converter::convert_blend_factor(descriptor.alpha.dstFactor);
    alphaOperation_ = converter::convert_blend_operation(descriptor.alpha.operation);
}

void GlesBlendState::apply() const {

    if (!descriptor_.blendingEnabled) {
        glDisable(GL_BLEND);
    } else {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(colorSrcFactor_, colorDstFactor_, alphaSrcFactor_, alphaDstFactor_);
        glBlendEquationSeparate(colorOperation_, alphaOperation_);
    }
}

} // namespace ris::gles
