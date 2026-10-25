#pragma once

#include "ris/core.hpp"
#include "ris/gles/gles_types.hpp"

/// `glGetError` helpers. ris-framework-ts checks `gl.getError()` inline (for example in
/// `createCompressedTexture2D`); these are the shared versions. They need the window's GL context.
namespace ris::gles::error_utilities {

/// The name of a GL error code: "GL_INVALID_ENUM", "GL_INVALID_VALUE", "GL_INVALID_OPERATION",
/// "GL_INVALID_FRAMEBUFFER_OPERATION", "GL_OUT_OF_MEMORY", "GL_NO_ERROR", or "unknown" for other values.
RIS_API const char* error_name(GlEnum error) noexcept;

/// Reads and discards every pending GL error.
/// @throws Error(NotInitialized) without a window.
RIS_API void clear_errors();

/// Throws if `glGetError` reports an error, and discards all other pending errors.
/// @param what the operation that was just performed, used in the message
///        ("OpenGL ES error 0x0502 in <what>").
/// @throws Error(Graphics) on a GL error, Error(NotInitialized) without a window.
RIS_API void check_error(const char* what);

} // namespace ris::gles::error_utilities
