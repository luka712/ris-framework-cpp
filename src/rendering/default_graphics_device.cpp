#include "ris/rendering/default_graphics_device.hpp"

// The only file outside of the backend folders that names a backend.
#include "ris/gles/gles_graphics_device.hpp"

namespace ris {

std::unique_ptr<AGraphicsDevice> create_default_graphics_device(Framework& framework, const GraphicsDeviceDescriptor& descriptor) {
    return std::make_unique<gles::GlesGraphicsDevice>(framework, descriptor);
}

} // namespace ris
