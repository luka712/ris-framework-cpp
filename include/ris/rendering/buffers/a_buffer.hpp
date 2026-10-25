#pragma once

#include <cstddef>
#include <string>

#include "ris/core.hpp"
#include "ris/rendering/buffers/buffer_usage.hpp"

namespace ris {

/// Base of all GPU buffers, like `IBuffer` of ris-framework-ts. This is an abstract, graphics API neutral class:
/// it names no graphics API types, so it can be implemented by any rendering backend (OpenGL ES in
/// `ris/gles/buffers/`, others later). Use buffers through ABufferFactory and the abstract classes
/// AVertexBuffer, AIndexBuffer and AUniformBuffer; the native object (e.g. the GL buffer name) is only reachable
/// on the concrete backend classes.
///
/// Rules every implementation follows:
///  - A buffer owns its GPU storage and releases it on destruction. Buffers are not copyable; concrete
///    classes are movable, abstract ones are normally held in a `std::unique_ptr` (see ABufferFactory). A
///    moved-from buffer is empty and must not be used again.
///  - The backend's device / context must be alive while a buffer is created, updated or destroyed, and all
///    calls must happen on the thread the device belongs to (for OpenGL ES: the thread of the ris::Window).
///  - Failures throw standard exceptions. Argument problems (empty data, bad stride, wrong usage flags, ...) are
///    reported with std::invalid_argument or std::domain_error before the device is touched, so they do
///    not need a window. A missing device is std::logic_error.
class RIS_API ABuffer {
public:
    virtual ~ABuffer() = default;

    /// The debug label given at creation, empty if none. Backends show it in graphics debuggers where possible.
    virtual const std::string& label() const noexcept = 0;
    /// Size of the GPU storage in bytes. Uniform buffers report their padded size.
    virtual size_t byte_size() const noexcept = 0;
    /// The usage flags: the flags given at creation plus the flag of the buffer kind (e.g. Index).
    virtual BufferUsageFlags usage() const noexcept = 0;

    ABuffer(const ABuffer&) = delete;
    ABuffer& operator=(const ABuffer&) = delete;

protected:
    ABuffer() = default;
    ABuffer(ABuffer&&) noexcept = default;
    ABuffer& operator=(ABuffer&&) noexcept = default;
};

} // namespace ris
