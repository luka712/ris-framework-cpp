#pragma once

#include "ris/core.hpp"
#include "ris/graphics.hpp"
#include "ris/rendering/primitive/front_face.hpp"

/// Face culling of the current context, like `WebGLCullingUtilities` of ris-framework-ts. They need the
/// window's GL context and throw std::logic_error without one.
namespace ris::gles::culling_utilities {

/// Sets the culling mode: `CullMode::None` disables `GL_CULL_FACE`, the other modes enable it and select
/// the face with `glCullFace`. The front face winding is not touched.
RIS_API void set_culling(CullMode mode);

/// Sets which winding order is the front face (`glFrontFace`). `FrontFace::Ccw` is the OpenGL default.
RIS_API void set_front_face(FrontFace front_face);

} // namespace ris::gles::culling_utilities
