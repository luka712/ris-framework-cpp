#include "ris/gles/utilities/sampler_utilities.hpp"

#include <algorithm>

#include "internal.hpp"
#include "ris/gles/utilities/anisotropic_filter_utilities.hpp"
#include "ris/gles/utilities/gles_debug_utilities.hpp"
#include "ris/gles/utilities/error_utilities.hpp"
#include "ris/gles/utilities/gles_converter.hpp"

using namespace ris::detail;

namespace ris::gles::sampler_utilities {

namespace {
constexpr GLenum kTextureMaxAnisotropy = 0x84FE;   // GL_EXT_texture_filter_anisotropic
} // namespace

uint32_t create(const SamplerParams& params, std::string_view label) {
    require_context();
    // Convert first: an unsupported value throws before any GL object exists.
    const GLenum min_filter = converter::convert_min_filter(params.min_filter, params.mip_map_filter);
    const GLenum mag_filter = converter::convert_mag_filter(params.mag_filter);
    const GLenum wrap_s = converter::convert_address_mode(params.address_mode_u);
    const GLenum wrap_t = converter::convert_address_mode(params.address_mode_v);
    const GLenum wrap_r = converter::convert_address_mode(params.address_mode_w);

    GLuint sampler = 0;
    glGenSamplers(1, &sampler);
    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(min_filter));
    glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(mag_filter));
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, static_cast<GLint>(wrap_s));
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, static_cast<GLint>(wrap_t));
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_R, static_cast<GLint>(wrap_r));
    if (params.compare_function != SamplerCompareFunction::Never) {
        glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_FUNC,
                            static_cast<GLint>(converter::convert_compare_function(params.compare_function)));
    } else {
        glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }
    if (params.anisotropy > 1.0f && anisotropic_filter_utilities::is_supported()) {
        const float level = std::min(params.anisotropy, anisotropic_filter_utilities::get_max_anisotropy());
        glSamplerParameterf(sampler, kTextureMaxAnisotropy, level);
    }

    try {
        error_utilities::check_error("sampler_utilities::create");
    } catch (...) {
        glDeleteSamplers(1, &sampler);
        throw;
    }
    debug_label_utilities::set_label(debug_label_utilities::ObjectKind::Sampler, sampler, label);
    return sampler;
}

void delete_sampler(uint32_t& sampler) noexcept {
    if (sampler != 0 && has_context()) {
        const GLuint id = sampler;
        glDeleteSamplers(1, &id);
    }
    sampler = 0;
}

} // namespace ris::gles::sampler_utilities
