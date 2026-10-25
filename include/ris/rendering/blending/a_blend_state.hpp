#pragma once

#include "ris/core.hpp"
#include "ris/rendering/blending/blend_state_descriptor.hpp"

namespace ris {

/// A blend state, like `IBlendState` of ris-framework-ts: the blending configuration created from a
/// BlendStateDescriptor, ready to be applied to the graphics pipeline. Abstract and graphics API neutral: every
/// rendering backend provides one implementation (GlesBlendState for OpenGL ES, which has `apply()`). Create
/// blend states with `AGraphicsDevice::createBlendState`; the device also owns a default one.
///
/// The blend state is owned by whoever created it (free it with `delete`, before the graphics device). It owns no
/// GPU resource.
class RIS_API ABlendState {
public:
    virtual ~ABlendState() = default;

    /// The descriptor the blend state was created from.
    virtual const BlendStateDescriptor& getDescriptor() const noexcept = 0;

    ABlendState(const ABlendState&) = delete;
    ABlendState& operator=(const ABlendState&) = delete;

protected:
    ABlendState() = default;
};

} // namespace ris
