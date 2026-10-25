#pragma once

#include <cstdint>

namespace ris {

/// The current state of a resource, such as a texture, like `State` of ris-framework-ts (same values). It tracks
/// the life cycle of the resource: it is created, initialized and finally disposed of.
enum class State : uint32_t {
    Created = 0,       ///< The object exists, but is not initialized yet.
    Initialized = 1,   ///< The object is initialized and can be used.
    Disposing = 2,     ///< The object is being disposed of, its destructor is running.
    Disposed = 3       ///< The object is disposed of. Its GPU resources are released, it must not be used.
};

} // namespace ris
