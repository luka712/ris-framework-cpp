#pragma once

#include <cstdint>
#include <string_view>

#include "ris/core.hpp"
#include "ris/gles/gles_types.hpp"
#include "ris/rendering/texture/texture_format.hpp"

/// Render buffers (off-screen depth / stencil storage for framebuffers), like
/// `WebGLRenderbufferUtilities` of ris-framework-ts. They need the window's GL context and throw
/// std::logic_error without one.
namespace ris::gles::renderbuffer_utilities {

/// Creates a render buffer with storage. `create` of ris-framework-ts.
/// @param format the internal format, for example `TextureFormat::Depth24Stencil8` (see
///        converter::convert_internal_format).
/// @param width  width in pixels, greater than zero.
/// @param height height in pixels, greater than zero.
/// @param label  optional debug label, see debug_label_utilities.
/// @return the render buffer name, to be deleted with delete_renderbuffer(). No render buffer stays bound.
/// @throws std::invalid_argument for a non-positive size, std::domain_error for a format without a GL
///         mapping, std::runtime_error if GL reports an error (the render buffer is deleted then).
RIS_API GlUint create(TextureFormat format, int32_t width, int32_t height, std::string_view label = {});

/// Deletes a render buffer and sets the name to 0. Does nothing for 0, or when the window (and context) is gone.
RIS_API void delete_renderbuffer(GlUint& renderbuffer) noexcept;

} // namespace ris::gles::renderbuffer_utilities
