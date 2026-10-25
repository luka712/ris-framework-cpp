#include "ris/gles/sampler/gles_sampler.hpp"

#include <stdexcept>
#include <string>

#include "internal.hpp"
#include "ris/gles/utilities/sampler_utilities.hpp"

using namespace ris::detail;

namespace ris::gles {

uint32_t GlesSampler::s_ids = 0;

GlesSampler::GlesSampler(const SamplerDescriptor& descriptor, const std::string& label) : descriptor_(descriptor) {
    if (!(descriptor.maxAnisotropy >= 1.0f)) { // also rejects NaN
        throw std::invalid_argument("GlesSampler: maxAnisotropy must be at least 1 (got " + std::to_string(descriptor.maxAnisotropy) + ")");
    }

    sampler_utilities::SamplerParams params;
    params.min_filter = descriptor.minFilter;
    params.mag_filter = descriptor.magFilter;
    params.mip_map_filter = descriptor.mipMapFilter;
    params.address_mode_u = descriptor.addressModeU;
    params.address_mode_v = descriptor.addressModeV;
    params.address_mode_w = descriptor.addressModeW;
    params.compare_function = descriptor.compareFunction;
    params.anisotropy = descriptor.maxAnisotropy;
    handle_ = sampler_utilities::create(params, label);

    id_ = ++s_ids;
}

GlesSampler::~GlesSampler() {
    sampler_utilities::delete_sampler(handle_);
}

void GlesSampler::bind(uint32_t unit) const {
    require_context();
    glBindSampler(unit, handle_);
    check_gl("GlesSampler::bind");
}

} // namespace ris::gles
