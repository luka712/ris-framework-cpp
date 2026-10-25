#include "ris/gles/buffers/gles_buffer_object.hpp"

#include <utility>

#include "ris/gles/utilities/debug_label_utilities.hpp"
#include "ris/rendering/buffers/buffer_validation.hpp"

namespace ris::gles {

GlesBufferObject::GlesBufferObject(BufferType type, BufferUsageFlags usage, const void* data, size_t byte_size,
                                   std::string label)
    : buffer_(type, has_flag(usage, BufferUsageFlags::CopyDst) ? BufferUsage::Dynamic : BufferUsage::Static, data,
              byte_size),
      usage_(usage),
      label_(std::move(label)) {
    debug_label_utilities::set_label(debug_label_utilities::ObjectKind::Buffer, buffer_.gl_handle(), label_);
}

void GlesBufferObject::write(size_t byte_offset, const void* data, size_t size, const char* what) {
    buffer_validation::check_update_allowed(usage_, what);
    // Buffer::update validates the range (and the pointer) and does nothing if it throws.
    buffer_.update(byte_offset, data, size);
}

} // namespace ris::gles
