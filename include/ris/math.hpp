#pragma once

// ris uses glm (https://github.com/g-truc/glm) for math. The types below are plain aliases, so any glm
// function works with the framework: glm::perspective, glm::lookAt, glm::rotate, glm::translate ...
// Matrices are column-major (OpenGL layout) and angles are in radians.

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ris {

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

} // namespace ris
