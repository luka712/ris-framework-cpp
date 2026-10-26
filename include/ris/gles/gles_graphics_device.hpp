#pragma once

#include <memory>

#include "ris/gles/gles_gpu_info.hpp"
#include "ris/gles/blending/gles_blend_state.hpp"
#include "ris/gles/gles_graphics_device_features.hpp"
#include "ris/gles/primitive/gles_primitive_state.hpp"
#include "ris/gles/sampler/gles_sampler.hpp"
#include "ris/rendering/a_graphics_device.hpp"
#include "ris/rendering/graphics_device_descriptor.hpp"

namespace ris::gles {

/// The OpenGL ES 3 implementation of AGraphicsDevice, like `WebGlGraphicsDevice` of ris-framework-ts. It creates
/// GlesSampler, GlesBlendState, GlesPrimitiveState, GlesRenderPass and GlesSwapChain (the `create...` results can be
/// cast to these with static_cast to reach the GL specific functions such as `apply()`). Create it directly, or
/// through create_default_graphics_device(); `Framework::graphicsDevice()` owns the one of the application.
/// Needs the window's GL context (so a ris::Window) from `initialize()` on, everything on the window's thread.
/// See AGraphicsDevice for the ownership rules and the exceptions.
///
/// Details:
///  - The GPU info is a GlesGpuInfo, it comes from `GL_RENDERER`, `GL_VENDOR` and `GL_VERSION`.
///  - The features come from the extensions, see GlesGraphicsDeviceFeatures.
///  - `setupDebugCallback()` uses `GL_KHR_debug`: the driver messages go to ris' log (`[gl] ...`, like the window's
///    `WindowDesc::debug` does). `pushDebugGroup` / `popDebugGroup` call `glPushDebugGroup` / `glPopDebugGroup` (OpenGL ES 3.2).
///  - `frameEnd()` does nothing: OpenGL ES keeps no per-frame resources to clean up. It exists for the backends
///    that have some.
///  - The default sampler uses `TextureFilteringPreset`: Nearest, Bilinear (linear, no mips) or Trilinear
///    (linear, linear mips) with repeat addressing.
class RIS_API GlesGraphicsDevice : public AGraphicsDevice {
public:
    /// Does not touch the GL context: call `initialize()` when the window exists.
    explicit GlesGraphicsDevice(const GraphicsDeviceDescriptor& descriptor = {});
    ~GlesGraphicsDevice() override;

    const AGpuInfo& getGpuInfo() const override;
    const AGraphicsDeviceFeatures& getFeatures() const override;
    const ASampler& getDefaultTextureSampler() const override;
    const ABlendState& getDefaultBlendState() const override;
    const APrimitiveState& getDefaultPrimitiveState() const override;

    void initialize() override;
    void setupDebugCallback() override;
    void frameEnd() override;

    ASampler* createSampler(const SamplerDescriptor& samplerDescriptor) override;
    ABlendState* createBlendState(const BlendStateDescriptor& descriptor) override;
    APrimitiveState* createPrimitiveState(const PrimitiveStateDescriptor& descriptor) override;
    ARenderPass* createRenderPass(const RenderPassDescriptor& descriptor) override;
    ASwapChain* createSwapChain(Window& window, const SwapChainDescriptor& swapChainDescription) override;

    void pushDebugGroup(const std::string& groupName) override;
    void popDebugGroup() override;

private:
    void requireInitialized(const char* what) const;

    GraphicsDeviceDescriptor descriptor_;
    bool initialized_ = false;
    std::unique_ptr<GlesGraphicsDeviceFeatures> m_features;   // created by initialize()
    std::unique_ptr<GlesGpuInfo> m_gpuInfo;                   // created by initialize(), after the features
    std::unique_ptr<GlesSampler> defaultTextureSampler_;
    std::unique_ptr<GlesBlendState> defaultBlendState_;
    std::unique_ptr<GlesPrimitiveState> defaultPrimitiveState_;
};

} // namespace ris::gles
