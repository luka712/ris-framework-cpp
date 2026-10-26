#include "ris/gles/gles_graphics_device.hpp"

#include <stdexcept>
#include <string>

#include "internal.hpp"

#include <EGL/egl.h>
#include <GLES2/gl2ext.h> // after internal.hpp: needs the GL types of <GLES3/gl3.h>
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>
#include <SDL3/SDL_log.h>

#include "ris/gles/render_pass/gles_render_pass.hpp"
#include "ris/gles/swap_chain/gles_swap_chain.hpp"
#include "ris/gles/utilities/gles_utilities.hpp"
#include  "ris/framework.hpp"
#include "window_manager.hpp"

using namespace ris::detail;

namespace ris::gles {
    namespace {
        // The GL_KHR_debug entry points. Extension functions are looked up through EGL, the core name is used by OpenGL ES 3.2
        // contexts, the KHR suffix by GL_KHR_debug on older ones (like debug_label_utilities).
        void (*findFunction(const char *khrName, const char *coreName))() {
            void (*p)() = eglGetProcAddress(khrName);
            return p ? p : eglGetProcAddress(coreName);
        }

        bool hasDebug() { return GlesUtilities::context().isExtensionSupported("GL_KHR_debug"); }

        void GL_APIENTRY debugCallback(GLenum, GLenum type, GLuint, GLenum severity, GLsizei, const GLchar *message,
                                       const void *) {
            const LogLevel level = (type == GL_DEBUG_TYPE_ERROR_KHR || severity == GL_DEBUG_SEVERITY_HIGH_KHR)
                                       ? LogLevel::Error
                                       : (severity == GL_DEBUG_SEVERITY_NOTIFICATION_KHR
                                              ? LogLevel::Trace
                                              : LogLevel::Warn);
            log(level, "[gl] %s", message);
        }

        SamplerDescriptor defaultSamplerDescriptor(TextureFilteringPreset preset) {
            SamplerDescriptor descriptor; // repeat addressing
            switch (preset) {
                case RIS_TEXTURE_FILTERING_NEAREST:
                    descriptor.minFilter = SamplerFilter::Nearest;
                    descriptor.magFilter = SamplerFilter::Nearest;
                    descriptor.mipMapFilter = MipMapSamplerFilter::None;
                    return descriptor;
                case RIS_TEXTURE_FILTERING_BILINEAR:
                    descriptor.mipMapFilter = MipMapSamplerFilter::None;
                    return descriptor;
                case RIS_TEXTURE_FILTERING_TRILINEAR:
                    descriptor.mipMapFilter = MipMapSamplerFilter::Linear;
                    return descriptor;
            }
            throw std::invalid_argument(
                "GlesGraphicsDevice: unknown texture filtering preset " + std::to_string(
                    static_cast<uint32_t>(preset)));
        }
    } // namespace

    GlesGraphicsDevice::GlesGraphicsDevice(Framework& framework, const GraphicsDeviceDescriptor &descriptor) :
    m_framework(framework),
    descriptor_(descriptor) {
    }

    GlesGraphicsDevice::~GlesGraphicsDevice() {
        // The GL objects of the device go before the context does.
        defaultPrimitiveState_.reset();
        defaultBlendState_.reset();
        defaultTextureSampler_.reset();
        m_gpuInfo.reset();
        m_features.reset();
        if (m_glContext) {
            SDL_GL_DestroyContext(m_glContext);
        }
    }

    void GlesGraphicsDevice::requireInitialized(const char *what) const {
        if (!initialized_) {
            throw std::logic_error(
                std::string("GlesGraphicsDevice::") + what +
                ": the graphics device is not initialized, call initialize() first");
        }
    }

    const AGpuInfo &GlesGraphicsDevice::getGpuInfo() const {
        requireInitialized("getGpuInfo");
        return *m_gpuInfo;
    }

    const AGraphicsDeviceFeatures &GlesGraphicsDevice::getFeatures() const {
        requireInitialized("getFeatures");
        return *m_features;
    }

    const ASampler &GlesGraphicsDevice::getDefaultTextureSampler() const {
        requireInitialized("getDefaultTextureSampler");
        return *defaultTextureSampler_;
    }

    const ABlendState &GlesGraphicsDevice::getDefaultBlendState() const {
        requireInitialized("getDefaultBlendState");
        return *defaultBlendState_;
    }

    const APrimitiveState &GlesGraphicsDevice::getDefaultPrimitiveState() const {
        requireInitialized("getDefaultPrimitiveState");
        return *defaultPrimitiveState_;
    }

    void GlesGraphicsDevice::initialize() {

        const auto sdlWindow = m_framework.windowManager().getWindow();
        m_glContext = SDL_GL_CreateContext(sdlWindow);
        if (!m_glContext) {
            SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
            SDL_DestroyWindow(sdlWindow);
            return;
        }

        if (initialized_) throw std::invalid_argument(
            "GlesGraphicsDevice::initialize: the graphics device is already initialized");

        m_features = std::make_unique<GlesGraphicsDeviceFeatures>();
        m_gpuInfo = std::make_unique<GlesGpuInfo>(*m_features);

        defaultTextureSampler_ = std::make_unique<GlesSampler>(
            defaultSamplerDescriptor(descriptor_.samplerFilteringPreset), "ris default texture sampler");
        defaultBlendState_ = std::make_unique<GlesBlendState>();
        defaultPrimitiveState_ = std::make_unique<GlesPrimitiveState>();

        initialized_ = true;
    }

    void GlesGraphicsDevice::setupDebugCallback() {
        requireInitialized("setupDebugCallback");

        if (!hasDebug()) return;
        auto callbackFn = reinterpret_cast<void (*)(GLDEBUGPROCKHR, const void *)>(findFunction("glDebugMessageCallbackKHR",
                                                                    "glDebugMessageCallback"));
        if (!callbackFn) return;
        glEnable(GL_DEBUG_OUTPUT_KHR);
        callbackFn(debugCallback, nullptr);
    }

    void GlesGraphicsDevice::frameEnd() {
        requireInitialized("frameEnd");
        // Nothing to clean up: OpenGL ES keeps no resources that live for one frame only.
    }

    ASampler *GlesGraphicsDevice::createSampler(const SamplerDescriptor &samplerDescriptor) {
        requireInitialized("createSampler");
        return new GlesSampler(samplerDescriptor);
    }

    ABlendState *GlesGraphicsDevice::createBlendState(const BlendStateDescriptor &descriptor) {
        requireInitialized("createBlendState");
        return new GlesBlendState(descriptor);
    }

    APrimitiveState *GlesGraphicsDevice::createPrimitiveState(const PrimitiveStateDescriptor &descriptor) {
        requireInitialized("createPrimitiveState");
        return new GlesPrimitiveState(descriptor);
    }

    ARenderPass *GlesGraphicsDevice::createRenderPass(const RenderPassDescriptor &descriptor) {
        requireInitialized("createRenderPass");
        return new GlesRenderPass(descriptor);
    }

    ASwapChain *GlesGraphicsDevice::createSwapChain(WindowManager &window, const SwapChainDescriptor &swapChainDescription) {
        requireInitialized("createSwapChain");
        return new GlesSwapChain(window, swapChainDescription);
    }

    void GlesGraphicsDevice::pushDebugGroup(const std::string &groupName) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, static_cast<GLsizei>(groupName.size()), groupName.c_str());
    }

    void GlesGraphicsDevice::popDebugGroup() {
        glPopDebugGroup();
    }
} // namespace ris::gles
