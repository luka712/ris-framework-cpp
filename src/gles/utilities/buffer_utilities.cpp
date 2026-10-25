#include "ris/gles/utilities/buffer_utilities.hpp"

#include "internal.hpp"
#include "ris/gles/utilities/debug_label_utilities.hpp"
#include "ris/gles/utilities/error_utilities.hpp"
#include "ris/gles/utilities/gles_converter.hpp"

using namespace ris::detail;

namespace ris::gles::buffer_utilities {

namespace {

/// The binding target used while creating / updating: index buffers must not touch the element array
/// binding of the current vertex array.
GLenum scratch_target(BufferType type) {
    return type == BufferType::Index ? GL_COPY_WRITE_BUFFER : converter::convert_buffer_target(type);
}

} // namespace

GlUint create_buffer(BufferType type, const void* data, size_t byte_size, GlEnum usage, std::string_view label) {
    require_context();
    if (byte_size == 0) {
        throw_error(Result::InvalidArgument, "create_buffer: size must be greater than zero");
    }
    GLuint id = 0;
    glGenBuffers(1, &id);
    const GLenum target = scratch_target(type);
    glBindBuffer(target, id);
    glBufferData(target, static_cast<GLsizeiptr>(byte_size), data, usage);
    glBindBuffer(target, 0);
    try {
        error_utilities::check_error("create_buffer");
    } catch (...) {
        glDeleteBuffers(1, &id);
        throw;
    }
    debug_label_utilities::set_label(debug_label_utilities::ObjectKind::Buffer, id, label);
    return id;
}

GlUint create_vertex_buffer(std::span<const float> data, BufferUsageFlags usage, std::string_view label) {
    return create_buffer(BufferType::Vertex, data.data(), data.size_bytes(), converter::convert_buffer_usage(usage), label);
}

GlUint create_vertex_buffer(size_t byte_size, BufferUsageFlags usage, std::string_view label) {
    return create_buffer(BufferType::Vertex, nullptr, byte_size, converter::convert_buffer_usage(usage), label);
}

GlUint create_index_buffer(std::span<const uint16_t> indices, BufferUsageFlags usage, std::string_view label) {
    return create_buffer(BufferType::Index, indices.data(), indices.size_bytes(), converter::convert_buffer_usage(usage),
                         label);
}

GlUint create_index_buffer(std::span<const uint32_t> indices, BufferUsageFlags usage, std::string_view label) {
    return create_buffer(BufferType::Index, indices.data(), indices.size_bytes(), converter::convert_buffer_usage(usage),
                         label);
}

GlUint create_uniform_buffer(std::span<const float> data, BufferUsageFlags usage, std::string_view label) {
    return create_buffer(BufferType::Uniform, data.data(), data.size_bytes(), converter::convert_buffer_usage(usage), label);
}

GlUint create_uniform_buffer(size_t byte_size, BufferUsageFlags usage, std::string_view label) {
    return create_buffer(BufferType::Uniform, nullptr, byte_size, converter::convert_buffer_usage(usage), label);
}

void update_buffer(BufferType type, GlUint buffer, size_t byte_offset, const void* data, size_t byte_size) {
    require_context();
    if (buffer == 0 || !data || byte_size == 0) {
        throw_error(Result::InvalidArgument, "update_buffer: needs a buffer, data and a non-empty range");
    }
    const GLenum target = scratch_target(type);
    glBindBuffer(target, buffer);
    glBufferSubData(target, static_cast<GLintptr>(byte_offset), static_cast<GLsizeiptr>(byte_size), data);
    glBindBuffer(target, 0);
    error_utilities::check_error("update_buffer");
}

void update_vertex_buffer(GlUint buffer, std::span<const float> data, size_t dest_byte_offset) {
    update_buffer(BufferType::Vertex, buffer, dest_byte_offset, data.data(), data.size_bytes());
}

void delete_buffer(GlUint& buffer) noexcept {
    if (buffer != 0 && has_context()) {
        const GLuint id = buffer;
        glDeleteBuffers(1, &id);
    }
    buffer = 0;
}

} // namespace ris::gles::buffer_utilities
