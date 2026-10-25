#pragma once

#include <cstdint>

namespace ris {

/// What happens to the contents of an attachment at the end of a render pass, like `StoreAction` of
/// ris-framework-ts.
enum class StoreAction : uint32_t {
    Store = 0,     ///< The contents are kept, so they can be read later (the default of colour attachments).
    Discard = 1    ///< The contents are discarded and undefined afterwards, which is faster on tile based GPUs. Use it
                   ///< when a later pass does not read the attachment (the default of depth and stencil).
};

} // namespace ris
