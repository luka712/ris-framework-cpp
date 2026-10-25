#include "ris/rendering/default_buffer_factory.hpp"

// The only file outside of the backend folders that names a backend.
#include "ris/gles/buffers/gles_buffer_factory.hpp"

namespace ris {

std::unique_ptr<ABufferFactory> create_default_buffer_factory() {
    return std::make_unique<gles::GlesBufferFactory>();
}

} // namespace ris
