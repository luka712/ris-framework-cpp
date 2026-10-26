//
// Created by luka on 25.10.26.
//

#ifndef RIS_FRAMEWORK_A_GRAPHICS_DEVICE_HPP
#define RIS_FRAMEWORK_A_GRAPHICS_DEVICE_HPP

#include <string>

#include "ris/core.hpp"
#include "ris/rendering/a_graphics_device_features.hpp"
#include "ris/rendering/blending/a_blend_state.hpp"
#include "ris/rendering/blending/blend_state_descriptor.hpp"
#include "ris/rendering/a_gpu_info.hpp"
#include "ris/rendering/primitive/a_primitive_state.hpp"
#include "ris/rendering/primitive/primitive_state_descriptor.hpp"
#include "ris/rendering/render_pass/a_render_pass.hpp"
#include "ris/rendering/render_pass/render_pass_descriptor.hpp"
#include "ris/rendering/sampler/a_sampler.hpp"
#include "ris/rendering/sampler/sampler_descriptor.hpp"
#include "ris/rendering/swap_chain/a_swap_chain.hpp"
#include "ris/rendering/swap_chain/swap_chain_descriptor.hpp"

namespace ris {

class Window;   // see ris/window.hpp

/// The graphics device, like `IGraphicsDevice` of ris-framework-ts: the entry point to the GPU. It reports what the
/// GPU can do, owns the default sampler, blend state and primitive state, and creates samplers, blend states,
/// primitive states, render passes and swap chains. Abstract and graphics API neutral: every rendering backend
/// provides one implementation (GlesGraphicsDevice for OpenGL ES). Get the device of the default backend from
/// `Framework::graphicsDevice()` or with create_default_graphics_device()
/// (ris/rendering/default_graphics_device.hpp).
///
/// Rules every implementation follows:
///  - `initialize()` must be called once, after the backend's context exists (for OpenGL ES: after the ris::Window
///    was created), before anything else. Everything runs on the thread that owns the context.
///  - The objects returned by the `create...` functions are owned by the caller: free them with `delete`, before
///    the device is destroyed. The default sampler, blend state and primitive state are owned by the device. The
///    device must be destroyed before the window.
///  - There is no `dispose()` as in TypeScript (`IDisposable`): the destructors release the GPU resources.
///  - Failures throw standard exceptions: std::logic_error without a context or an initialised device,
///    std::invalid_argument for bad arguments, std::domain_error for values the backend does not support and
///    std::runtime_error if the backend fails.
class RIS_API AGraphicsDevice {
public:
    virtual ~AGraphicsDevice() = default;

    /// The info about the GPU. Owned by the device.
    /// @throws std::logic_error if the device was not initialised.
    virtual const AGpuInfo& getGpuInfo() const = 0;

    /// The features of the graphics device.
    /// @throws std::logic_error if the device was not initialised.
    virtual const AGraphicsDeviceFeatures& getFeatures() const = 0;

    /// The default sampler of the graphics device, created from the `samplerFilteringPreset` of the
    /// GraphicsDeviceDescriptor. Owned by the device.
    /// @throws std::logic_error if the device was not initialised.
    virtual const ASampler& getDefaultTextureSampler() const = 0;

    /// The default blend state of the graphics device (alpha blending, see BlendStateDescriptor). All render
    /// pipelines will use this blend state by default. Owned by the device.
    /// @throws std::logic_error if the device was not initialised.
    virtual const ABlendState& getDefaultBlendState() const = 0;

    /// The default primitive state of the graphics device (triangle list, back face culling, counter clockwise
    /// front faces, see PrimitiveStateDescriptor). All render pipelines will use this primitive state by default.
    /// Owned by the device.
    /// @throws std::logic_error if the device was not initialised.
    virtual const APrimitiveState& getDefaultPrimitiveState() const = 0;

    /// Initializes the graphics device: queries the GPU info and features and creates the default sampler, blend
    /// state and primitive state.
    /// @throws std::logic_error without a context, std::invalid_argument if it was already initialised.
    virtual void initialize() = 0;

    /// Sets up the debug callback for the graphics device. This enables the debug output if it is supported, and routes
    /// the debug messages of the GPU driver to the log of the framework (see set_log_callback). Does nothing if the
    /// backend has no debug output. Safe to call more than once.
    /// @throws std::logic_error if the device was not initialised.
    virtual void setupDebugCallback() = 0;

    /// Called at the end of each frame to perform any necessary cleanup of resources that were used during the frame.
    /// @throws std::logic_error if the device was not initialised.
    virtual void frameEnd() = 0;

    /// Creates a sampler object.
    /// @param samplerDescriptor the sampler descriptor.
    /// @return the sampler, owned by the caller (never null).
    /// @throws std::invalid_argument for a maximum anisotropy below 1, std::domain_error for values the backend does
    ///         not support, std::logic_error if the device was not initialised.
    virtual ASampler* createSampler(const SamplerDescriptor& samplerDescriptor) = 0;

    /// Creates a blend state. Not part of `IGraphicsDevice` of ris-framework-ts, but of its abstract base class.
    /// @param descriptor the blend state descriptor.
    /// @return the blend state, owned by the caller (never null).
    /// @throws std::domain_error for values the backend does not support, std::logic_error if the device was not
    ///         initialised.
    virtual ABlendState* createBlendState(const BlendStateDescriptor& descriptor) = 0;

    /// Creates a primitive state for the graphics device.
    /// @param descriptor the primitive state descriptor.
    /// @return the created primitive state, owned by the caller (never null).
    /// @throws std::domain_error for values the backend does not support, std::logic_error if the device was not
    ///         initialised.
    virtual APrimitiveState* createPrimitiveState(const PrimitiveStateDescriptor& descriptor) = 0;

    /// Creates a render pass for the graphics device.
    /// @param descriptor the descriptor for creating a render pass.
    /// @return the created render pass, owned by the caller (never null). The textures and swap chains of the
    ///         attachments are not owned by it and must outlive it.
    /// @throws std::invalid_argument for a bad attachment configuration (no attachment, neither or both of
    ///         swap chain and texture, a swap chain together with other attachments), std::runtime_error if the
    ///         backend fails, std::logic_error if the device was not initialised.
    virtual ARenderPass* createRenderPass(const RenderPassDescriptor& descriptor) = 0;

    /// Creates a swap chain for the graphics device. The swap chain is used to present the rendered image to the
    /// screen. Replaces the `window: any` of ris-framework-ts (a canvas there, which is a ris::Window here).
    /// @param window the window to present to. The swap chain must not outlive it.
    /// @param swapChainDescription the swap chain descriptor.
    /// @return the swap chain instance, owned by the caller (never null).
    /// @throws std::invalid_argument for a negative size, std::domain_error for a present mode the backend does not
    ///         support, std::logic_error if the device was not initialised.
    virtual ASwapChain* createSwapChain(Window& window, const SwapChainDescriptor& swapChainDescription) = 0;

    /// Pushes a debug group with the specified name onto the graphics device's debug stack. The commands until the
    /// matching popDebugGroup() appear as a group in graphics debuggers (RenderDoc, ...). Pushes and pops must be balanced.
    /// @param groupName the name of the debug group.
    virtual void pushDebugGroup(const std::string& groupName) = 0;

    /// Pops the current debug group from the graphics device's debug stack, ending the current debug group and
    /// returning to the previous one.
    virtual void popDebugGroup() = 0;

    AGraphicsDevice(const AGraphicsDevice&) = delete;
    AGraphicsDevice& operator=(const AGraphicsDevice&) = delete;

protected:
    AGraphicsDevice() = default;
};

} // namespace ris

#endif //RIS_FRAMEWORK_A_GRAPHICS_DEVICE_HPP
