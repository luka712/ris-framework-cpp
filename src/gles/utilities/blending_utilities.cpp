#include "ris/gles/utilities/blending_utilities.hpp"

#include "internal.hpp"
#include "ris/gles/utilities/gles_converter.hpp"

using namespace ris::detail;

namespace ris::gles::blending_utilities {

void enable_blend(const BlendParams& params) {

    glEnable(GL_BLEND);
    glBlendFuncSeparate(converter::convert_blend_factor(params.color.src_factor),
                        converter::convert_blend_factor(params.color.dst_factor),
                        converter::convert_blend_factor(params.alpha.src_factor),
                        converter::convert_blend_factor(params.alpha.dst_factor));
    glBlendEquationSeparate(converter::convert_blend_operation(params.color.operation),
                            converter::convert_blend_operation(params.alpha.operation));
}

void disable_blend() {

    glDisable(GL_BLEND);
}

} // namespace ris::gles::blending_utilities
