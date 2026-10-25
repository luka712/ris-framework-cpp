#pragma once

#include <cstdint>

#include "ris/core.hpp"
#include "ris/rendering/sampler/mip_map_sampler_filter.hpp"
#include "ris/rendering/sampler/sampler_address_mode.hpp"
#include "ris/rendering/sampler/sampler_filter.hpp"

namespace ris {

/// The sampler defines how a texture is sampled in shaders (filtering, address mode), like `ISampler` of
/// ris-framework-ts. Abstract and graphics API neutral: every rendering backend provides one implementation
/// (GlesSampler for OpenGL ES). Create samplers with `AGraphicsDevice::createSampler`.
///
/// The sampler is owned by whoever created it (free it with `delete`, before the graphics device). The
/// native handle (the GL sampler name) is only reachable on the concrete backend class.
class RIS_API ASampler {
public:
    virtual ~ASampler() = default;

    /// The id of the sampler, unique among the samplers of the process. 1, 2, 3, ...
    virtual uint32_t getId() const noexcept = 0;
    /// The min filter of the sampler.
    virtual SamplerFilter getMinFilter() const noexcept = 0;
    /// The mag filter of the sampler.
    virtual SamplerFilter getMagFilter() const noexcept = 0;
    /// The mipmap filter of the sampler.
    virtual MipMapSamplerFilter getMipmapFilter() const noexcept = 0;
    /// The address mode of the sampler for the U coordinate.
    virtual SamplerAddressMode getAddressModeU() const noexcept = 0;
    /// The address mode of the sampler for the V coordinate.
    virtual SamplerAddressMode getAddressModeV() const noexcept = 0;
    /// The address mode of the sampler for the W coordinate.
    virtual SamplerAddressMode getAddressModeW() const noexcept = 0;

    ASampler(const ASampler&) = delete;
    ASampler& operator=(const ASampler&) = delete;

protected:
    ASampler() = default;
};

} // namespace ris
