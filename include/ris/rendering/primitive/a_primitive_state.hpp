#pragma once

#include "ris/core.hpp"
#include "ris/rendering/primitive/cull_mode.hpp"
#include "ris/rendering/primitive/front_face.hpp"
#include "ris/rendering/primitive/primitive_topology.hpp"

namespace ris {

/// The primitive state, like `IPrimitiveState` of ris-framework-ts: describes the primitives to be rendered
/// and how. Abstract and graphics API neutral: every rendering backend provides one implementation
/// (GlesPrimitiveState for OpenGL ES, which has `apply()`). Create primitive states with
/// `AGraphicsDevice::createPrimitiveState`; the device also owns a default one.
///
/// The primitive state is owned by whoever created it (free it with `delete`, before the graphics device). It owns
/// no GPU resource.
class RIS_API APrimitiveState {
public:
    virtual ~APrimitiveState() = default;

    /// The topology of the primitive.
    virtual PrimitiveTopology getTopology() const noexcept = 0;
    /// The culling mode.
    virtual CullMode getCullFace() const noexcept = 0;
    /// The front face winding order.
    virtual FrontFace getFrontFace() const noexcept = 0;

    APrimitiveState(const APrimitiveState&) = delete;
    APrimitiveState& operator=(const APrimitiveState&) = delete;

protected:
    APrimitiveState() = default;
};

} // namespace ris
