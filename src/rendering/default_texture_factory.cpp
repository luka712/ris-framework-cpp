#include "ris/rendering/default_texture_factory.hpp"

// The only file outside of the backend folders that names a backend.
#include "ris/gles/texture/gles_texture_factory.hpp"

namespace ris {

std::unique_ptr<ATextureFactory> create_default_texture_factory() {
    return std::make_unique<gles::GlesTextureFactory>();
}

} // namespace ris
