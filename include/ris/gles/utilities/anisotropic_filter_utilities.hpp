#pragma once

#include "ris/core.hpp"
#include "ris/gles/gles_types.hpp"

/// Anisotropic texture filtering through `GL_EXT_texture_filter_anisotropic`, like
/// `WebGLFilterAnisotropicUtilities` of ris-framework-ts. They need the window's GL context and throw
/// Error(NotInitialized) without one.
namespace ris::gles::anisotropic_filter_utilities {

/// True if `GL_EXT_texture_filter_anisotropic` is available.
RIS_API bool is_supported();

/// The maximum supported level of anisotropic filtering, 0 if the extension is not supported.
RIS_API float get_max_anisotropy();

/// Sets the anisotropy level of a 2D texture (`GL_TEXTURE_MAX_ANISOTROPY_EXT`). Does nothing if the
/// extension is not supported. The level is clamped to [1, get_max_anisotropy()]. The texture binding of
/// `GL_TEXTURE_2D` is restored afterwards (the TypeScript version leaves the texture bound).
/// @param texture    a 2D texture name.
/// @param anisotropy the level, 1 is no anisotropic filtering, typical values are 4, 8 and 16.
RIS_API void set_anisotropy(GlUint texture, float anisotropy);

} // namespace ris::gles::anisotropic_filter_utilities
