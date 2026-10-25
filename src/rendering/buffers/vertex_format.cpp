#include "ris/rendering/buffers/vertex_format.hpp"

namespace ris {

namespace {

uint32_t attribute_components(VertexFormat f) {
    switch (f) {
        case VertexFormat::Float32: return 1;
        case VertexFormat::Float32x2: return 2;
        case VertexFormat::Float32x3: return 3;
        case VertexFormat::Float32x4: return 4;
        case VertexFormat::Float32x16: return 16;
    }
    return 0;
}

} // namespace

uint32_t vertex_format_byte_size(VertexFormat format) noexcept { return attribute_components(format) * 4u; }

} // namespace ris
