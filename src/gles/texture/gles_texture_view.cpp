#include "ris/gles/texture/gles_texture_view.hpp"

#include <utility>

#include "ris/gles/texture/gles_texture2d.hpp"

namespace ris::gles {

GlesTextureView::GlesTextureView(GlesTexture2D& texture, TextureViewDimension dimension, TextureFormat format,
                                 std::string label)
    : texture_(&texture), dimension_(dimension), format_(format), label_(std::move(label)) {}

void GlesTextureView::bind(uint32_t unit) const {
    texture_->bind(unit);
}

} // namespace ris::gles
