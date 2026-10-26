#pragma once

#include "ris/core.hpp"
#include <cstdint>

/// Linking shader programs, like `WebGLProgramUtilities` of ris-framework-ts. They need the window's GL context.
namespace ris::gles::program_utilities {

/// Links a vertex and a fragment shader into a program. `create` of ris-framework-ts.
///
/// The shaders are detached again after linking (so they can be deleted right away) but not deleted.
/// @param vertex_shader   a compiled vertex shader, see shader_utilities.
/// @param fragment_shader a compiled fragment shader.
/// @return the program name.
/// @throws std::runtime_error with the info log ("shader link error: ..."), the program is deleted;
///         std::logic_error without a window.
RIS_API uint32_t create(uint32_t vertex_shader, uint32_t fragment_shader);

/// Deletes a program and sets the name to 0. Does nothing for 0, or when the window (and context) is gone.
RIS_API void delete_program(uint32_t& program) noexcept;

} // namespace ris::gles::program_utilities
