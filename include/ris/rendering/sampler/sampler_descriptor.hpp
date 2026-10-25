#pragma once

#include "ris/rendering/sampler/mip_map_sampler_filter.hpp"
#include "ris/rendering/sampler/sampler_address_mode.hpp"
#include "ris/rendering/sampler/sampler_compare_function.hpp"
#include "ris/rendering/sampler/sampler_filter.hpp"

namespace ris {

/// The descriptor for creating a sampler, like `SamplerDescriptor` of ris-framework-ts. Contains the
/// parameters of `AGraphicsDevice::createSampler`, with the defaults of ris-framework-ts.
struct SamplerDescriptor {
    /// How texture coordinates outside [0, 1] are handled in the U direction.
    SamplerAddressMode addressModeU = SamplerAddressMode::Repeat;
    /// How texture coordinates outside [0, 1] are handled in the V direction.
    SamplerAddressMode addressModeV = SamplerAddressMode::Repeat;
    /// How texture coordinates outside [0, 1] are handled in the W direction.
    SamplerAddressMode addressModeW = SamplerAddressMode::Repeat;
    /// How the texture is sampled when it is minified (smaller than the area it is mapped to).
    SamplerFilter minFilter = SamplerFilter::Linear;
    /// How the texture is sampled when it is magnified (larger than the area it is mapped to).
    SamplerFilter magFilter = SamplerFilter::Linear;
    /// How the mip levels are selected when the texture has several levels of detail.
    MipMapSamplerFilter mipMapFilter = MipMapSamplerFilter::None;
    /// `Never` samples normally, `LessEqual` makes it a shadow (depth comparison) sampler. Not in the
    /// TypeScript descriptor (its WebGL sampler always uses `Never`).
    SamplerCompareFunction compareFunction = SamplerCompareFunction::Never;
    /// The maximum anisotropy, 1 (default) is no anisotropic filtering. Backends clamp it to what the GPU
    /// supports and ignore it without support. Must be at least 1. Not in the TypeScript descriptor.
    float maxAnisotropy = 1.0f;
};

} // namespace ris
