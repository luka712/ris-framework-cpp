#pragma once

#include <cstdint>

namespace ris {

/// What a sampler does with texture coordinates outside [0, 1], like `SamplerAddressMode` of
/// ris-framework-ts. (The older `ris::Wrap` of graphics.hpp has the first three values.)
enum class SamplerAddressMode : uint32_t {
    Repeat = 0,         ///< The texture repeats.
    MirrorRepeat = 1,   ///< The texture repeats, mirrored at every other repetition.
    ClampToEdge = 2,    ///< The edge texel is used.
    ClampToBorder = 3   ///< A border colour is used. OpenGL ES 3.0 has no border: it clamps to the edge.
};

} // namespace ris
