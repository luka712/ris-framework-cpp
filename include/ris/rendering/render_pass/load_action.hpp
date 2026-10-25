#pragma once

#include <cstdint>

namespace ris {

/// What happens to the contents of an attachment at the beginning of a render pass, like `LoadAction` of
/// ris-framework-ts.
enum class LoadAction : uint32_t {
    Clear = 0,       ///< The attachment is cleared to the clear value (the default). Cheap on tile based GPUs.
    Load = 1,        ///< The existing contents are kept.
    Undefined = 2    ///< The contents are undefined: fastest if every pixel is written before it is read. Backends
                     ///< without "don't care" fall back to `Load` (OpenGL ES does).
};

} // namespace ris
