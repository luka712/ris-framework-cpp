#pragma once

#include <memory>

#include "ris/core.hpp"
#include "ris/rendering/buffers/a_buffer_factory.hpp"

namespace ris {

/// The one place that selects the rendering backend for buffers: creates the buffer factory of the backend the
/// framework is built for, currently OpenGL ES (GlesBufferFactory). Code that wants to stay independent of the
/// backend, like Mesh, asks for its buffers here and programs against ABufferFactory and the abstract buffer
/// classes. The factory is stateless and cheap, so call it whenever one is needed.
///
/// A future backend is added by implementing ABufferFactory in `ris/<api>/buffers/` and returning it from this
/// function (src/rendering/default_buffer_factory.cpp), chosen at build time or from the active window.
RIS_API std::unique_ptr<ABufferFactory> create_default_buffer_factory();

} // namespace ris
