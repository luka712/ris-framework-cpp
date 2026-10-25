#pragma once

#include <string_view>

#include "ris/core.hpp"
#include "ris/gles/gles_types.hpp"
#include "ris/rendering/sampler/mip_map_sampler_filter.hpp"
#include "ris/rendering/sampler/sampler_address_mode.hpp"
#include "ris/rendering/sampler/sampler_compare_function.hpp"
#include "ris/rendering/sampler/sampler_filter.hpp"

/// GL sampler objects (OpenGL ES 3.0), like `WebGlSamplerUtilities` of ris-framework-ts. They need the
/// window's GL context and throw Error(NotInitialized) without one.
namespace ris::gles::sampler_utilities {

/// The parameters of create(). The defaults are those of ris-framework-ts.
struct SamplerParams {
    SamplerFilter min_filter = SamplerFilter::Linear;
    SamplerFilter mag_filter = SamplerFilter::Linear;
    MipMapSamplerFilter mip_map_filter = MipMapSamplerFilter::None;
    SamplerAddressMode address_mode_u = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode address_mode_v = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode address_mode_w = SamplerAddressMode::ClampToEdge;
    /// `Never` samples normally, `LessEqual` makes it a shadow sampler (`GL_COMPARE_REF_TO_TEXTURE`).
    SamplerCompareFunction compare_function = SamplerCompareFunction::Never;
    /// 1 is no anisotropic filtering. Higher values are used only if `GL_EXT_texture_filter_anisotropic`
    /// is available (they are ignored otherwise) and are clamped to the maximum of the device.
    float anisotropy = 1.0f;
};

/// Creates a sampler object. `create` of ris-framework-ts.
/// @param params the sampler state.
/// @param label  optional debug label, see debug_label_utilities.
/// @return the sampler name, to be deleted with delete_sampler().
RIS_API GlUint create(const SamplerParams& params = {}, std::string_view label = {});

/// Deletes a sampler and sets the name to 0. Does nothing for 0, or when the window (and context) is gone.
RIS_API void delete_sampler(GlUint& sampler) noexcept;

} // namespace ris::gles::sampler_utilities
