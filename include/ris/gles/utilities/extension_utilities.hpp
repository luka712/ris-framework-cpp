#pragma once

#include "ris/core.hpp"

/// OpenGL ES extension queries. ris-framework-ts calls `gl.getExtension(name)` in several places; this is
/// the native counterpart. They need the window's GL context.
namespace ris::gles::extension_utilities {

/// True if the context exposes the extension, for example "GL_KHR_texture_compression_astc_ldr". Matches
/// whole names only ("GL_EXT_foo" does not match "GL_EXT_foo_bar").
/// @throws std::logic_error without a window.
RIS_API bool has_extension(const char* name);

} // namespace ris::gles::extension_utilities
