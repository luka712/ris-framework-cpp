#include "ris/rendering/buffers/buffer.hpp"

#include <utility>

#include "internal.hpp"
#include "rendering/buffers/buffers_internal.hpp"

using namespace ris::detail;

namespace ris {

namespace detail {

namespace {
constexpr uint32_t kind_mask = static_cast<uint32_t>(BufferUsageFlags::Vertex | BufferUsageFlags::Index |
                                                     BufferUsageFlags::Uniform);
} // namespace

/// Validates `usage` for a buffer of the kind `own` and returns the flags to store (given flags + own).
BufferUsageFlags resolve_usage(BufferUsageFlags usage, BufferUsageFlags own, const char* what) {
    if (has_flag(usage, BufferUsageFlags::Storage) || has_flag(usage, BufferUsageFlags::MapRead)) {
        throw_error(Result::Unsupported, "%s: Storage and MapRead buffers are not available in OpenGL ES 3.0", what);
    }
    if ((static_cast<uint32_t>(usage) & kind_mask & ~static_cast<uint32_t>(own)) != 0) {
        throw_error(Result::InvalidArgument, "%s: usage 0x%x contains the flag of another buffer kind", what,
                    static_cast<unsigned>(usage));
    }
    return usage | own;
}

} // namespace detail

// ---- BufferBase -----------------------------------------------------------------------------

BufferBase::BufferBase(BufferType type, BufferUsageFlags usage, const void* data, size_t byte_size, std::string label)
    : buffer_(type, has_flag(usage, BufferUsageFlags::CopyDst) ? BufferUsage::Dynamic : BufferUsage::Static, data,
              byte_size),
      usage_(usage),
      label_(std::move(label)) {}

void BufferBase::write(size_t byte_offset, const void* data, size_t size, const char* what) {
    if (!has_flag(usage_, BufferUsageFlags::CopyDst)) {
        throw_error(Result::InvalidArgument, "%s: the buffer was not created with BufferUsageFlags::CopyDst", what);
    }
    // Buffer::update validates the range (and the pointer) and does nothing if it throws.
    buffer_.update(byte_offset, data, size);
}

} // namespace ris
