#pragma once

#include <algorithm>
#include <cstddef>
#include <span>
#include <vector>

/// Aligning values, like `AlignUtilities` of ris-framework-ts. Header only.
namespace ris::align_utilities {

/// Rounds `value` up to the next multiple of `alignment`.
///
/// Works for any alignment, not only powers of two (ASTC blocks are 5x5, 6x6, ...), unlike the bit trick of
/// the TypeScript version. An alignment of 0 returns `value` unchanged.
/// @param value     the value to align.
/// @param alignment the alignment.
/// @return the smallest multiple of `alignment` that is greater than or equal to `value`.
[[nodiscard]] constexpr size_t align(size_t value, size_t alignment) noexcept {
    if (alignment == 0) return value;
    return (value + alignment - 1) / alignment * alignment;
}

/// Copies `values` into a vector whose length is a multiple of `alignment` elements, padded with zeros.
/// `alignArray` of ris-framework-ts, which returns the input itself if it is already aligned; a C++ caller
/// can compare the sizes if it wants to avoid the copy.
/// @param values    the data.
/// @param alignment the alignment in elements.
template <typename T>
[[nodiscard]] std::vector<T> align_array(std::span<const T> values, size_t alignment) {
    std::vector<T> result(align(values.size(), alignment), T{});
    std::copy(values.begin(), values.end(), result.begin());
    return result;
}

} // namespace ris::align_utilities
