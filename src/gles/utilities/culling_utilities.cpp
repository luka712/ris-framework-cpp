#include "ris/gles/utilities/culling_utilities.hpp"

#include "internal.hpp"
#include "ris/gles/utilities/gles_converter.hpp"

using namespace ris::detail;

namespace ris::gles::culling_utilities {

void set_culling(CullMode mode) {
    if (mode == CullMode::None) {
        glDisable(GL_CULL_FACE);
        return;
    }
    glEnable(GL_CULL_FACE);
    glCullFace(converter::convert_cull_face(mode));
}

void set_front_face(FrontFace front_face) {
    glFrontFace(converter::convert_front_face(front_face));
}

} // namespace ris::gles::culling_utilities
