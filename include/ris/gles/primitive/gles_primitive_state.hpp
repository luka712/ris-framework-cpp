#pragma once

#include "ris/gles/gles_types.hpp"
#include "ris/rendering/primitive/a_primitive_state.hpp"
#include "ris/rendering/primitive/primitive_state_descriptor.hpp"

namespace ris::gles {

/// The OpenGL ES implementation of APrimitiveState, like `WebGlPrimitiveState` of ris-framework-ts: the cull
/// face, front face and the draw mode converted to GL enums. Creating it needs no GL context, `apply()` does.
class RIS_API GlesPrimitiveState : public APrimitiveState {
public:
    /// @throws std::domain_error for enum values without OpenGL ES mapping.
    explicit GlesPrimitiveState(const PrimitiveStateDescriptor& descriptor = {});

    PrimitiveTopology getTopology() const noexcept override { return descriptor_.topology; }
    CullMode getCullFace() const noexcept override { return descriptor_.cullFace; }
    FrontFace getFrontFace() const noexcept override { return descriptor_.frontFace; }

    /// The draw mode of the topology (`GL_TRIANGLES`, `GL_LINES`, ...) for `glDrawArrays` / `glDrawElements`.
    GlEnum getGlPrimitiveType() const noexcept { return primitiveType_; }

    /// Applies the culling to the GL context: `glEnable(GL_CULL_FACE)` with `glCullFace` and `glFrontFace`, or
    /// `glDisable(GL_CULL_FACE)` for `CullMode::None`. The topology is not GL state, it is used by the draw call.
    /// @throws std::logic_error without a window.
    void apply() const;

private:
    PrimitiveStateDescriptor descriptor_;
    GlEnum primitiveType_ = 0;
    GlEnum cullFace_ = 0;
    GlEnum frontFace_ = 0;
};

} // namespace ris::gles
