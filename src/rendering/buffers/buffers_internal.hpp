#pragma once

// Internal helpers shared by the translation units of the typed buffers. Not installed.

#include "ris/rendering/buffers/buffer_usage.hpp"

namespace ris::detail {

/// Validates `usage` for a buffer of the kind `own` (one of Vertex, Index, Uniform) and returns the
/// flags to store (given flags + own).
/// @throws Error(Unsupported) for Storage / MapRead, Error(InvalidArgument) for the flag of another kind.
/// @param what name of the calling buffer type, used in the error message.
BufferUsageFlags resolve_usage(BufferUsageFlags usage, BufferUsageFlags own, const char* what);

} // namespace ris::detail
