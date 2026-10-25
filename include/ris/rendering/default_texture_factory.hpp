#pragma once

#include <memory>

#include "ris/core.hpp"
#include "ris/rendering/texture/a_texture_factory.hpp"

namespace ris {

/// The one place that selects the rendering backend for textures: creates the texture factory of the backend the
/// framework is built for, currently OpenGL ES (GlesTextureFactory). Like create_default_buffer_factory(). The
/// factory is stateless and cheap.
RIS_API std::unique_ptr<ATextureFactory> create_default_texture_factory();

} // namespace ris
