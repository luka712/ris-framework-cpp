#include "ris/gles/primitive/gles_primitive_state.hpp"

#include "internal.hpp"
#include "ris/gles/utilities/gles_converter.hpp"

using namespace ris::detail;

namespace ris::gles {

GlesPrimitiveState::GlesPrimitiveState(const PrimitiveStateDescriptor& descriptor) : descriptor_(descriptor) {
    primitiveType_ = converter::convert_primitive_topology(descriptor.topology);
    cullFace_ = converter::convert_cull_face(descriptor.cullFace);
    frontFace_ = converter::convert_front_face(descriptor.frontFace);
}

void GlesPrimitiveState::apply() const {

    if (descriptor_.cullFace == CullMode::None) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace(cullFace_);
    }
    glFrontFace(frontFace_);

}

} // namespace ris::gles
