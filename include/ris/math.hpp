#pragma once

// ris uses glm (https://github.com/g-truc/glm) for math: glm::vec2, glm::vec3, glm::vec4, glm::mat3 and glm::mat4 are used
// directly, with every glm function: glm::perspective, glm::lookAt, glm::rotate, glm::translate ...
// Matrices are column-major (OpenGL layout) and angles are in radians.

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
