#pragma once

#include "ris/framework_configuration.h"

namespace ris {

/// The configuration of a graphics device, like `IGraphicsDeviceDescriptor` of ris-framework-ts. The `alpha` and
/// `powerPreference` options of the TypeScript descriptor belong to the creation of the window / context here, see
/// WindowDesc.
struct GraphicsDeviceDescriptor {
    /// The filtering of the default texture sampler (`AGraphicsDevice::getDefaultTextureSampler`). It is used when a
    /// texture is sampled without a specific sampler. Trilinear by default.
    TextureFilteringPreset samplerFilteringPreset = RIS_TEXTURE_FILTERING_TRILINEAR;
};

} // namespace ris
