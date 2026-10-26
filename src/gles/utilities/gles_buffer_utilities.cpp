#include "ris/gles/utilities/gles_buffer_utilities.hpp"

#include <stdexcept>
#include <string>

#include "internal.hpp"
#include "ris/gles/utilities/gles_debug_utilities.hpp"
#include "ris/gles/utilities/error_utilities.hpp"
#include "ris/gles/utilities/gles_converter.hpp"

using namespace ris::detail;

namespace ris::gles {

namespace {

/// The binding target used while creating / updating: index buffers must not touch the element array
/// binding of the current vertex array.
GLenum scratchTarget(BufferType type) {
    return type == BufferType::Index ? GL_COPY_WRITE_BUFFER : converter::convert_buffer_target(type);
}

/// The kind of buffer as used in the "Failed to create ..." message.
const char* kindName(BufferType type) {
    switch (type) {
        case BufferType::Vertex: return "vertex buffer";
        case BufferType::Index: return "index buffer";
        case BufferType::Uniform: return "uniform buffer";
    }
    return "buffer";
}

} // namespace

uint32_t GlesBufferUtilities::createBuffer(BufferType type, const void* data, size_t byteSize, uint32_t usage,
                                         std::string_view label) const {

    if (byteSize == 0) {
        throw std::invalid_argument("createBuffer: size must be greater than zero");
    }
    GLuint id = 0;
    glGenBuffers(1, &id);
    if (id == 0) {
        throw std::runtime_error(std::string("Failed to create ") + kindName(type) + ".");
    }
    const GLenum target = scratchTarget(type);
    glBindBuffer(target, id);
    glBufferData(target, static_cast<GLsizeiptr>(byteSize), data, usage);
    glBindBuffer(target, 0);
    try {
        error_utilities::check_error("createBuffer");
    } catch (...) {
        glDeleteBuffers(1, &id);
        throw;
    }
    debug_label_utilities::set_label(debug_label_utilities::ObjectKind::Buffer, id, label);
    return id;
}

uint32_t GlesBufferUtilities::createVertexBuffer(std::span<const float> data, BufferUsageFlags usage,
                                               std::string_view label) const {
    return createBuffer(BufferType::Vertex, data.data(), data.size_bytes(), converter::convert_buffer_usage(usage), label);
}

uint32_t GlesBufferUtilities::createVertexBuffer(size_t byteSize, BufferUsageFlags usage, std::string_view label) const {
    return createBuffer(BufferType::Vertex, nullptr, byteSize, converter::convert_buffer_usage(usage), label);
}

uint32_t GlesBufferUtilities::createIndexBuffer(std::span<const uint16_t> indices, BufferUsageFlags usage,
                                              std::string_view label) const {
    return createBuffer(BufferType::Index, indices.data(), indices.size_bytes(), converter::convert_buffer_usage(usage), label);
}

uint32_t GlesBufferUtilities::createIndexBuffer(std::span<const uint32_t> indices, BufferUsageFlags usage,
                                              std::string_view label) const {
    return createBuffer(BufferType::Index, indices.data(), indices.size_bytes(), converter::convert_buffer_usage(usage), label);
}

uint32_t GlesBufferUtilities::createIndexBuffer(size_t byteSize, BufferUsageFlags usage, std::string_view label) const {
    return createBuffer(BufferType::Index, nullptr, byteSize, converter::convert_buffer_usage(usage), label);
}

uint32_t GlesBufferUtilities::createUniformBuffer(std::span<const float> data, BufferUsageFlags usage,
                                                std::string_view label) const {
    return createBuffer(BufferType::Uniform, data.data(), data.size_bytes(), converter::convert_buffer_usage(usage), label);
}

uint32_t GlesBufferUtilities::createUniformBuffer(std::span<const uint8_t> data, BufferUsageFlags usage,
                                                std::string_view label) const {
    return createBuffer(BufferType::Uniform, data.data(), data.size_bytes(), converter::convert_buffer_usage(usage), label);
}

uint32_t GlesBufferUtilities::createUniformBuffer(size_t byteSize, BufferUsageFlags usage,
                                                std::string_view label) const {
    return createBuffer(BufferType::Uniform, nullptr, byteSize, converter::convert_buffer_usage(usage), label);
}

void GlesBufferUtilities::updateBuffer(BufferType type, uint32_t buffer, size_t byteOffset, const void* data,
                                       size_t byteSize) const {

    if (buffer == 0 || !data || byteSize == 0) {
        throw std::invalid_argument("updateBuffer: needs a buffer, data and a non-empty range");
    }
    const GLenum target = scratchTarget(type);
    glBindBuffer(target, buffer);
    glBufferSubData(target, static_cast<GLintptr>(byteOffset), static_cast<GLsizeiptr>(byteSize), data);
    glBindBuffer(target, 0);
    error_utilities::check_error("updateBuffer");
}

void GlesBufferUtilities::updateVertexBuffer(uint32_t buffer, std::span<const float> data, size_t destOffset,
                                             size_t srcOffset, size_t srcLength) const {
    if (srcOffset > data.size()) {
        throw std::out_of_range("updateVertexBuffer: srcOffset " + std::to_string(srcOffset) + " is beyond the "
                                + std::to_string(data.size()) + " floats of data");
    }
    const size_t length = srcLength > 0 ? srcLength : data.size() - srcOffset;
    if (length > data.size() - srcOffset) {
        throw std::out_of_range("updateVertexBuffer: range [" + std::to_string(srcOffset) + ", "
                                + std::to_string(srcOffset + length) + ") is beyond the " + std::to_string(data.size())
                                + " floats of data");
    }
    updateBuffer(BufferType::Vertex, buffer, destOffset, data.data() + srcOffset, length * sizeof(float));
}

void GlesBufferUtilities::deleteBuffer(uint32_t& buffer) const noexcept {
    if (buffer != 0) {
        const GLuint id = buffer;
        glDeleteBuffers(1, &id);
    }
    buffer = 0;
}

} // namespace ris::gles
