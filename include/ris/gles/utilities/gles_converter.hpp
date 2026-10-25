#pragma once

#include <cstdint>

#include "ris/core.hpp"
#include "ris/gles/gles_types.hpp"
#include "ris/graphics.hpp"
#include "ris/rendering/blending/blend_factor.hpp"
#include "ris/rendering/blending/blend_operation.hpp"
#include "ris/rendering/buffers/buffer_usage.hpp"
#include "ris/rendering/buffers/index_buffer_type.hpp"
#include "ris/rendering/buffers/vertex_format.hpp"
#include "ris/rendering/primitive/front_face.hpp"
#include "ris/rendering/primitive/primitive_topology.hpp"
#include "ris/rendering/sampler/mip_map_sampler_filter.hpp"
#include "ris/rendering/sampler/sampler_address_mode.hpp"
#include "ris/rendering/sampler/sampler_compare_function.hpp"
#include "ris/rendering/sampler/sampler_filter.hpp"
#include "ris/rendering/texture/texture_format.hpp"

/// Conversions from the framework's enums to OpenGL ES enumerants, like `WebGlConverter` of
/// ris-framework-ts (which converts to WebGL 2 enums, the same values as OpenGL ES 3.0).
///
/// All functions are pure: they need no window or GL context. Where the TypeScript converter throws
/// "NotImplementedException" for a value, these throw std::domain_error, which in practice only happens for
/// TextureFormat values with no OpenGL ES mapping.
namespace ris::gles::converter {

// ---- primitives, culling ----------------------------------------------------------------------

/// `Topology` to the draw mode (`GL_TRIANGLES`, `GL_LINES`, ...). `convertPrimitiveType` of ris-framework-ts.
RIS_API GlEnum convert_primitive_type(Topology topology);

/// `PrimitiveTopology` (the neutral enum of PrimitiveStateDescriptor) to the draw mode, the same values as
/// convert_primitive_type(Topology). `convertPrimitiveType` of ris-framework-ts.
RIS_API GlEnum convert_primitive_topology(PrimitiveTopology topology);

/// `CullMode` to the face to cull (`GL_BACK`, `GL_FRONT`). `CullMode::None` gives `GL_NONE` (0), the
/// culling is switched off with `glDisable(GL_CULL_FACE)` instead (see culling_utilities).
/// `convertCullFace` / `convertCulling` of ris-framework-ts.
RIS_API GlEnum convert_cull_face(CullMode cull_mode);

/// `FrontFace` to `GL_CW` / `GL_CCW`. `convertFrontFace` of ris-framework-ts.
RIS_API GlEnum convert_front_face(FrontFace front_face);

// ---- buffers ----------------------------------------------------------------------------------

/// Usage flags to the `glBufferData` usage hint: `GL_DYNAMIC_DRAW` if `BufferUsageFlags::CopyDst` is set,
/// otherwise `GL_STATIC_DRAW`. `convertBufferUsage` of ris-framework-ts.
RIS_API GlEnum convert_buffer_usage(BufferUsageFlags usage);

/// The low level `BufferUsage` to `GL_STATIC_DRAW`, `GL_DYNAMIC_DRAW` or `GL_STREAM_DRAW`.
RIS_API GlEnum convert_buffer_usage(BufferUsage usage);

/// The kind of buffer to its natural binding target: `GL_ARRAY_BUFFER`, `GL_ELEMENT_ARRAY_BUFFER` or
/// `GL_UNIFORM_BUFFER`.
RIS_API GlEnum convert_buffer_target(BufferType type);

/// `IndexBufferType` to `GL_UNSIGNED_SHORT` / `GL_UNSIGNED_INT`.
RIS_API GlEnum convert_index_type(IndexBufferType type);

/// The low level `IndexType` to `GL_UNSIGNED_SHORT` / `GL_UNSIGNED_INT`.
/// @throws std::invalid_argument for `IndexType::None`.
RIS_API GlEnum convert_index_type(IndexType type);

// ---- vertex attributes ------------------------------------------------------------------------

/// The number of components of a vertex format (1, 2, 3, 4, or 16 for `Float32x16`), what
/// `convertVertexFormat` of ris-framework-ts returns. Note that `glVertexAttribPointer` takes at most 4
/// components: a `Float32x16` is four attributes, see VertexBufferLayout::to_vertex_attribs.
RIS_API uint32_t convert_vertex_format(VertexFormat format);

/// The component type of a vertex format. Always `GL_FLOAT` for now.
RIS_API GlEnum convert_vertex_format_type(VertexFormat format) noexcept;

/// The low level `AttribType` to `GL_FLOAT`, `GL_UNSIGNED_BYTE`, ...
RIS_API GlEnum convert_attrib_type(AttribType type);

// ---- blending, depth --------------------------------------------------------------------------

/// `BlendOperation` to `GL_FUNC_ADD`, `GL_FUNC_SUBTRACT`, `GL_FUNC_REVERSE_SUBTRACT`, `GL_MIN`, `GL_MAX`.
RIS_API GlEnum convert_blend_operation(BlendOperation operation);

/// `BlendFactor` to `GL_ONE`, `GL_SRC_ALPHA`, ...
RIS_API GlEnum convert_blend_factor(BlendFactor factor);

/// `CompareFunc` to `GL_NEVER` ... `GL_ALWAYS`, for `glDepthFunc`.
RIS_API GlEnum convert_compare_func(CompareFunc func);

/// `SamplerCompareFunction` to the `GL_TEXTURE_COMPARE_FUNC` value (`GL_NEVER`, `GL_LEQUAL`).
RIS_API GlEnum convert_compare_function(SamplerCompareFunction func);

// ---- textures, samplers -----------------------------------------------------------------------

/// `TextureFormat` to the sized internal format (`GL_RGBA8`, `GL_SRGB8_ALPHA8`, `GL_DEPTH_COMPONENT32F`,
/// `GL_DEPTH24_STENCIL8`, or the compressed format). `convertInternalFormat` of ris-framework-ts.
/// @throws std::domain_error for Undefined, Red32Float and Red32Typeless.
RIS_API GlEnum convert_internal_format(TextureFormat format);

/// `TextureFormat` to the pixel format of an upload (`GL_RGBA`, `GL_DEPTH_COMPONENT`, `GL_DEPTH_STENCIL`).
/// `convertToPixelFormat` of ris-framework-ts.
/// @throws std::domain_error for compressed and unmapped formats (they are uploaded without a pixel format).
RIS_API GlEnum convert_to_pixel_format(TextureFormat format);

/// `TextureFormat` to the pixel type of an upload (`GL_UNSIGNED_BYTE`, `GL_FLOAT`, `GL_UNSIGNED_INT_24_8`).
/// `convertToTextureType` of ris-framework-ts.
/// @throws std::domain_error like convert_to_pixel_format.
RIS_API GlEnum convert_to_texture_type(TextureFormat format);

/// True for BC3, BC7, ETC2 and ASTC formats.
RIS_API bool is_compressed_format(TextureFormat format) noexcept;

/// `SamplerFilter` to the magnification filter. `convertMagFilter` of ris-framework-ts.
RIS_API GlEnum convert_mag_filter(SamplerFilter filter);

/// A texel filter and a mip filter to the minification filter (`GL_LINEAR_MIPMAP_LINEAR`, ...).
/// `MipMapSamplerFilter::None` gives the plain `GL_NEAREST` / `GL_LINEAR`. `convertMinFilter` of
/// ris-framework-ts.
RIS_API GlEnum convert_min_filter(SamplerFilter filter, MipMapSamplerFilter mip_map_filter);

/// `SamplerAddressMode` to `GL_REPEAT`, `GL_MIRRORED_REPEAT` or `GL_CLAMP_TO_EDGE`. OpenGL ES 3.0 has no
/// border colour, so `ClampToBorder` gives `GL_CLAMP_TO_EDGE`, like the TypeScript converter.
/// `convertAddressMode` of ris-framework-ts.
RIS_API GlEnum convert_address_mode(SamplerAddressMode mode);

/// The low level `Filter` (texel + mip filter in one value) to the min filter value.
RIS_API GlEnum convert_filter(Filter filter);

/// The low level `Wrap` to `GL_REPEAT`, `GL_CLAMP_TO_EDGE` or `GL_MIRRORED_REPEAT`.
RIS_API GlEnum convert_wrap(Wrap wrap);

} // namespace ris::gles::converter
