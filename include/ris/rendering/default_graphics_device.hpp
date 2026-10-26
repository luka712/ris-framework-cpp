#pragma once

#include <memory>

#include "ris/core.hpp"
#include "ris/rendering/a_graphics_device.hpp"
#include "ris/rendering/graphics_device_descriptor.hpp"

namespace ris {

class Framework;

/// The one place that selects the rendering backend for the graphics device: creates the graphics device of the
/// backend the framework is built for, currently OpenGL ES (GlesGraphicsDevice). Like create_default_buffer_factory().
/// The device is returned uninitialised: call `initialize()` once the window exists (the Framework does that).
RIS_API std::unique_ptr<AGraphicsDevice> create_default_graphics_device(Framework& framework, const GraphicsDeviceDescriptor& descriptor = {});

} // namespace ris
