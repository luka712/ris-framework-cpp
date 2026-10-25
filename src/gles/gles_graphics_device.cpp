#include "ris/gles/gles_graphics_device.hpp"

#include <stdexcept>
#include <string>

#include "internal.hpp"

#include <EGL/egl.h>
#include <GLES2/gl2ext.h> // after internal.hpp: needs the GL types of <GLES3/gl3.h>

#include "ris/gles/render_pass/gles_render_pass.hpp"
#include "ris/gles/swap_chain/gles_swap_chain.hpp"
#include "ris/gles/utilities/extension_utilities.hpp"

using namespace ris::detail;

namespace ris::gles {

namespace {

std::string glString(GLenum name) {
    const GLubyte* s = glGetString(name);
    return s ? reinterpret_cast<const char*>(s) : "";
}

// The GL_KHR_debug entry points. Extension functions are looked up through EGL, the core name is used by OpenGL ES 3.2
// contexts, the KHR suffix by GL_KHR_debug on older ones (like debug_label_utilities).
void (*findFunction(const char* khrName, const char* coreName))() {
    void (*p)() = eglGetProcAddress(khrName);
    return p ? p : eglGetProcAddress(coreName);
}

bool hasDebug() { return extension_utilities::has_extension("GL_KHR_debug"); }

void GL_APIENTRY debugCallback(GLenum, GLenum type, GLuint, GLenum severity, GLsizei, const GLchar* message, const void*) {
    const LogLevel level = (type == GL_DEBUG_TYPE_ERROR_KHR || severity == GL_DEBUG_SEVERITY_HIGH_KHR)
                               ? LogLevel::Error
                               : (severity == GL_DEBUG_SEVERITY_NOTIFICATION_KHR ? LogLevel::Trace : LogLevel::Warn);
    log(level, "[gl] %s", message);
}

SamplerDescriptor defaultSamplerDescriptor(TextureFilteringPreset preset) {
    SamplerDescriptor descriptor; // repeat addressing
    switch (preset) {
        case TextureFilteringPreset::Nearest:
            descriptor.minFilter = SamplerFilter::Nearest;
            descriptor.magFilter = SamplerFilter::Nearest;
            descriptor.mipMapFilter = MipMapSamplerFilter::None;
            return descriptor;
        case TextureFilteringPreset::Bilinear:
            descriptor.mipMapFilter = MipMapSamplerFilter::None;
            return descriptor;
        case TextureFilteringPreset::Trilinear:
            descriptor.mipMapFilter = MipMapSamplerFilter::Linear;
            return descriptor;
    }
    throw std::invalid_argument("GlesGraphicsDevice: unknown texture filtering preset " + std::to_string(static_cast<uint32_t>(preset)));
}

} // namespace

GlesGraphicsDevice::GlesGraphicsDevice(const GraphicsDeviceDescriptor& descriptor) : descriptor_(descriptor) {}

GlesGraphicsDevice::~GlesGraphicsDevice() = default;

void GlesGraphicsDevice::requireInitialized(const char* what) const {
    if (!initialized_) {
        throw std::logic_error(std::string("GlesGraphicsDevice::") + what + ": the graphics device is not initialized, call initialize() first");
    }
}

const GpuInfo& GlesGraphicsDevice::getGpuInfo() const {
    requireInitialized("getGpuInfo");
    return gpuInfo_;
}

const AGraphicsDeviceFeatures& GlesGraphicsDevice::getFeatures() const {
    requireInitialized("getFeatures");
    return *features_;
}

const ASampler& GlesGraphicsDevice::getDefaultTextureSampler() const {
    requireInitialized("getDefaultTextureSampler");
    return *defaultTextureSampler_;
}

const ABlendState& GlesGraphicsDevice::getDefaultBlendState() const {
    requireInitialized("getDefaultBlendState");
    return *defaultBlendState_;
}

const APrimitiveState& GlesGraphicsDevice::getDefaultPrimitiveState() const {
    requireInitialized("getDefaultPrimitiveState");
    return *defaultPrimitiveState_;
}

void GlesGraphicsDevice::initialize() {
    require_context();
    if (initialized_) throw std::invalid_argument("GlesGraphicsDevice::initialize: the graphics device is already initialized");

    gpuInfo_ = {};
    gpuInfo_.name = glString(GL_RENDERER);
    gpuInfo_.vendor = glString(GL_VENDOR);
    gpuInfo_.apiVersion = glString(GL_VERSION);
    gpuInfo_.shadingLanguageVersion = glString(GL_SHADING_LANGUAGE_VERSION);
    gpuInfo_.api = RenderingBackend::OpenGLES;
    gpuInfo_.underlyingBackend = RenderingBackend::Default; // not reported through GL

    features_ = std::make_unique<GlesGraphicsDeviceFeatures>();
    if (features_->supportsTextureCompressionASTC()) gpuInfo_.supportedCompressedTextureFormats.push_back(TextureFormat::Astc4x4Rgba);
    if (features_->supportsTextureCompressionS3TC()) gpuInfo_.supportedCompressedTextureFormats.push_back(TextureFormat::Bc3RgbaUnorm);
    if (features_->supportsTextureCompressionETC2()) gpuInfo_.supportedCompressedTextureFormats.push_back(TextureFormat::Etc2Rgba8Unorm);
    if (features_->supportsTextureCompressionBC()) gpuInfo_.supportedCompressedTextureFormats.push_back(TextureFormat::Bc7RgbaUnorm);

    defaultTextureSampler_ = std::make_unique<GlesSampler>(defaultSamplerDescriptor(descriptor_.samplerFilteringPreset), "ris default texture sampler");
    defaultBlendState_ = std::make_unique<GlesBlendState>();
    defaultPrimitiveState_ = std::make_unique<GlesPrimitiveState>();

    initialized_ = true;
}

void GlesGraphicsDevice::setupDebugCallback() {
    requireInitialized("setupDebugCallback");
    require_context();
    if (!hasDebug()) return;
    using CallbackFn = void (*)(GLDEBUGPROCKHR, const void*);
    auto callbackFn = reinterpret_cast<CallbackFn>(findFunction("glDebugMessageCallbackKHR", "glDebugMessageCallback"));
    if (!callbackFn) return;
    glEnable(GL_DEBUG_OUTPUT_KHR);
    callbackFn(debugCallback, nullptr);
}

void GlesGraphicsDevice::frameEnd() {
    requireInitialized("frameEnd");
    // Nothing to clean up: OpenGL ES keeps no resources that live for one frame only.
}

ASampler* GlesGraphicsDevice::createSampler(const SamplerDescriptor& samplerDescriptor) {
    requireInitialized("createSampler");
    return new GlesSampler(samplerDescriptor);
}

ABlendState* GlesGraphicsDevice::createBlendState(const BlendStateDescriptor& descriptor) {
    requireInitialized("createBlendState");
    return new GlesBlendState(descriptor);
}

APrimitiveState* GlesGraphicsDevice::createPrimitiveState(const PrimitiveStateDescriptor& descriptor) {
    requireInitialized("createPrimitiveState");
    return new GlesPrimitiveState(descriptor);
}

ARenderPass* GlesGraphicsDevice::createRenderPass(const RenderPassDescriptor& descriptor) {
    requireInitialized("createRenderPass");
    return new GlesRenderPass(descriptor);
}

ASwapChain* GlesGraphicsDevice::createSwapChain(Window& window, const SwapChainDescriptor& swapChainDescription) {
    requireInitialized("createSwapChain");
    return new GlesSwapChain(window, swapChainDescription);
}

void GlesGraphicsDevice::pushDebugGroup(const std::string& groupName) {
    requireInitialized("pushDebugGroup");
    require_context();
    // The depth is counted even without GL_KHR_debug, so push and pop stay balanced.
    ++debugGroupDepth_;
    if (!hasDebug()) return;
    using PushFn = void (*)(GLenum, GLuint, GLsizei, const GLchar*);
    auto pushFn = reinterpret_cast<PushFn>(findFunction("glPushDebugGroupKHR", "glPushDebugGroup"));
    if (pushFn) pushFn(GL_DEBUG_SOURCE_APPLICATION_KHR, 0, static_cast<GLsizei>(groupName.size()), groupName.c_str());
}

void GlesGraphicsDevice::popDebugGroup() {
    requireInitialized("popDebugGroup");
    require_context();
    if (debugGroupDepth_ == 0) throw std::invalid_argument("GlesGraphicsDevice::popDebugGroup: there is no debug group to pop");
    --debugGroupDepth_;
    if (!hasDebug()) return;
    using PopFn = void (*)();
    auto popFn = reinterpret_cast<PopFn>(findFunction("glPopDebugGroupKHR", "glPopDebugGroup"));
    if (popFn) popFn();
}

} // namespace ris::gles
