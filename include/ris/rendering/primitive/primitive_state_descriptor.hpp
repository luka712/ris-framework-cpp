#pragma once

#include "ris/rendering/primitive/cull_mode.hpp"
#include "ris/rendering/primitive/front_face.hpp"
#include "ris/rendering/primitive/primitive_topology.hpp"

namespace ris {

/// Describes the state of a primitive, like `PrimitiveStateDescriptor` of ris-framework-ts.
struct PrimitiveStateDescriptor {
    /// The topology of the primitive, triangle list by default.
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    /// The culling mode, back faces are culled by default.
    CullMode cullFace = CullMode::Back;
    /// The winding order of front faces, counter clockwise by default.
    FrontFace frontFace = FrontFace::Ccw;

    constexpr bool operator==(const PrimitiveStateDescriptor&) const = default;
};

} // namespace ris
