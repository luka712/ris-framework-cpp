#pragma once

#include <span>
#include <string_view>

#include "ris/core.hpp"
#include "ris/gles/gles_types.hpp"
#include "ris/gles/utilities/depth_attachment_type.hpp"
#include "ris/rendering/texture/texture_format.hpp"

/// Creating framebuffers for render-to-texture, like `WebGlFrameBufferUtilities` of ris-framework-ts. They
/// need the window's GL context and throw std::logic_error without one.
///
/// Unlike the TypeScript version, the framebuffer that was bound before a call is bound again afterwards
/// (the TypeScript `create` leaves the new framebuffer bound, the others bind `null`). Every failing
/// function deletes the framebuffer it created before it throws.
namespace ris::gles::framebuffer_utilities {

/// Creates a framebuffer with the textures as colour attachments 0, 1, ... `create` of ris-framework-ts.
/// With more than one texture, `glDrawBuffers` is set up to write to all of them (the TypeScript version
/// does not do that, which only matters for multiple render targets).
/// @param color_textures 2D textures, at least one, none of them 0.
/// @param label          optional debug label.
/// @return the framebuffer name, to be deleted with delete_framebuffer().
/// @throws std::invalid_argument for no textures or a 0 texture, std::runtime_error if the framebuffer is
///         incomplete ("Framebuffer is not complete (status 0x...)").
RIS_API GlUint create(std::span<const GlUint> color_textures, std::string_view label = {});

/// Creates a framebuffer with one colour texture. See the span overload.
RIS_API GlUint create(GlUint color_texture, std::string_view label = {});

/// Creates a framebuffer with a colour texture (attachment 0) and a render buffer as depth and/or stencil
/// buffer. `createWithRenderBufferAsDepthBuffer` of ris-framework-ts.
/// @param color_texture a 2D texture.
/// @param depth_type    which attachment point the render buffer is used for.
/// @param render_buffer a render buffer with a matching format (see renderbuffer_utilities).
/// @param label         optional debug label.
RIS_API GlUint create_with_renderbuffer_as_depth_buffer(GlUint color_texture, DepthAttachmentType depth_type,
                                                        GlUint render_buffer, std::string_view label = {});

/// Creates a framebuffer with a colour texture (attachment 0) and a texture as depth and/or stencil buffer.
/// `createWithRenderTextureAsDepthBuffer` of ris-framework-ts.
/// @param color_texture a 2D texture.
/// @param depth_type    which attachment point the depth texture is used for.
/// @param depth_texture a 2D texture with a depth or depth-stencil format.
/// @param label         optional debug label.
RIS_API GlUint create_with_texture_as_depth_buffer(GlUint color_texture, DepthAttachmentType depth_type,
                                                   GlUint depth_texture, std::string_view label = {});

/// Attaches a render buffer as depth buffer of a framebuffer. `attachDepthStencilRenderBuffer` of
/// ris-framework-ts. Depth32Float attaches to the depth, Depth24Stencil8 to the depth-stencil attachment point.
/// @throws std::domain_error for any other format.
RIS_API void attach_depth_stencil_renderbuffer(GlUint framebuffer, GlUint render_buffer, TextureFormat format);

/// Attaches a texture as depth buffer of a framebuffer. `attachDepthStencilTexture` of ris-framework-ts.
/// @throws std::domain_error for a format other than Depth32Float and Depth24Stencil8.
RIS_API void attach_depth_stencil_texture(GlUint framebuffer, GlUint depth_texture, TextureFormat format);

/// The attachment point that belongs to a depth format: Depth32Float is Depth, Depth24Stencil8 is DepthStencil.
/// @throws std::domain_error for any other format.
RIS_API DepthAttachmentType depth_attachment_type_for(TextureFormat format);

/// Binds a framebuffer to `GL_FRAMEBUFFER` (0 is the window's back buffer). The viewport is not changed.
/// `ris::RenderTarget::bind()` also sets the viewport, use that for rendering to a RenderTarget.
RIS_API void bind(GlUint framebuffer);

/// Deletes a framebuffer and sets the name to 0. Does nothing for 0, or when the window (and context) is gone.
RIS_API void delete_framebuffer(GlUint& framebuffer) noexcept;

} // namespace ris::gles::framebuffer_utilities
