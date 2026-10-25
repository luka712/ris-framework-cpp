#pragma once

#include <cstdint>
#include <string>

#include "ris/gles/gles_types.hpp"
#include "ris/rendering/sampler/a_sampler.hpp"
#include "ris/rendering/sampler/sampler_descriptor.hpp"

namespace ris::gles {

/// The OpenGL ES implementation of ASampler, like `WebGlSampler` of ris-framework-ts: owns a GL sampler object
/// (see sampler_utilities). Needs the window's GL context for its whole life (create, bind, destroy).
/// Usually created by GlesGraphicsDevice::createSampler().
///
/// Deviations: `ClampToBorder` clamps to the edge (the OpenGL ES 3.0 core has no border colour), the anisotropy
/// is ignored without `GL_EXT_texture_filter_anisotropic`.
class RIS_API GlesSampler : public ASampler {
public:
    /// Creates the GL sampler object.
    /// @param descriptor the sampler descriptor.
    /// @param label      optional debug label, shown by graphics debuggers (KHR_debug).
    /// @throws std::invalid_argument for a maximum anisotropy below 1 (or NaN), std::domain_error for enum values
    ///         without OpenGL ES mapping, std::logic_error without a window.
    explicit GlesSampler(const SamplerDescriptor& descriptor = {}, const std::string& label = {});
    ~GlesSampler() override;

    uint32_t getId() const noexcept override { return id_; }
    SamplerFilter getMinFilter() const noexcept override { return descriptor_.minFilter; }
    SamplerFilter getMagFilter() const noexcept override { return descriptor_.magFilter; }
    MipMapSamplerFilter getMipmapFilter() const noexcept override { return descriptor_.mipMapFilter; }
    SamplerAddressMode getAddressModeU() const noexcept override { return descriptor_.addressModeU; }
    SamplerAddressMode getAddressModeV() const noexcept override { return descriptor_.addressModeV; }
    SamplerAddressMode getAddressModeW() const noexcept override { return descriptor_.addressModeW; }

    /// The descriptor the sampler was created from.
    const SamplerDescriptor& getDescriptor() const noexcept { return descriptor_; }

    /// The GL sampler object name, for interop with native code. Owned by this object.
    GlUint getGlHandle() const noexcept { return handle_; }

    /// Binds the sampler to a texture unit (`glBindSampler`): textures bound to the unit are sampled with this
    /// sampler instead of their own sampler state, until another sampler (or 0) is bound to the unit.
    /// @throws std::logic_error without a window.
    void bind(uint32_t unit) const;

private:
    static uint32_t s_ids;

    uint32_t id_ = 0;
    SamplerDescriptor descriptor_;
    GlUint handle_ = 0;
};

} // namespace ris::gles
