/*
 * The configuration of the framework, shared by the C++ `ris::Framework` and the C API (ris.h). Plain C, so it
 * can be included from C and C++.
 */
#ifndef RIS_FRAMEWORK_CONFIGURATION_H
#define RIS_FRAMEWORK_CONFIGURATION_H

#include <stdint.h>

/** The texture filtering of the default texture sampler, like `TextureSamplerFilteringPreset` of ris-framework-ts. */
typedef enum TextureFilteringPreset {
    RIS_TEXTURE_FILTERING_NEAREST = 0,    /* point filtering: nearest texel for minification and magnification, no mip mapping */
    RIS_TEXTURE_FILTERING_BILINEAR = 1,   /* linear filtering, no mip interpolation */
    RIS_TEXTURE_FILTERING_TRILINEAR = 2   /* linear filtering with linear mip interpolation */
} TextureFilteringPreset;

/// The framework configuration.
typedef struct FrameworkConfiguration {
    /** The title. If not passed in it will be set to "Ris Framework". */
    const char* title;
    /** The width of back buffer. By default 800. */
    uint32_t backBufferWidth;
    /** The width of back buffer. By default 600. */
    uint32_t backBufferHeight;
    /** The texture filtering preset. By default set to Trilinear. */
    TextureFilteringPreset textureFiltering;
} RisFrameworkConfiguration;

#endif /* RIS_FRAMEWORK_CONFIGURATION_H */
