#pragma once

#include <string>

#include "ris/core.hpp"
#include "ris/gles/gles_types.hpp"
#include "ris/shader/shader_stage.hpp"

/// Compiling shader stages, like `WebGlShaderUtilities` of ris-framework-ts. The returned names are owned by
/// the caller: delete them with `glDeleteShader` once the program is linked (see program_utilities), or
/// with delete_shader(). They need the window's GL context.
namespace ris::gles::shader_utilities {

/// Compiles one shader stage. `create` of ris-framework-ts.
/// @param source GLSL ES 3.00 source (`#version 300 es`).
/// @param stage  Vertex or Fragment.
/// @return the shader name.
/// @throws Error(Compile) with the info log (the message starts with "vertex shader compile error:" or
///         "fragment shader compile error:"), Error(Unsupported) for other stages, Error(NotInitialized)
///         without a window.
RIS_API GlUint create(const std::string& source, ShaderStage stage);

/// Compiles a vertex shader. `createVertexShader` of ris-framework-ts.
RIS_API GlUint create_vertex_shader(const std::string& source);

/// Compiles a fragment shader. `createFragmentShader` of ris-framework-ts.
RIS_API GlUint create_fragment_shader(const std::string& source);

/// Deletes a shader and sets the name to 0. Does nothing for 0, or when the window (and context) is gone.
RIS_API void delete_shader(GlUint& shader) noexcept;

} // namespace ris::gles::shader_utilities
