#pragma once

#include <cstdint>

namespace ris {

/// The element type of an IndexBuffer, like `IndexBufferType` of ris-framework-ts.
enum class IndexBufferType : uint32_t {
    UInt16 = 0,   ///< 2 byte unsigned indices.
    UInt32 = 1    ///< 4 byte unsigned indices.
};

} // namespace ris
