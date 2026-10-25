#pragma once

#include <string>
#include <vector>

#include "ris/core.hpp"
#include "ris/rendering/rendering_backend.hpp"
#include "ris/rendering/texture/texture_format.hpp"

namespace ris {

/// The GPU information, like `IGPUInfo` of ris-framework-ts: the name, vendor, API version and so on of the GPU.
/// Abstract and graphics API neutral: every rendering backend provides one implementation (GlesGpuInfo for
/// OpenGL ES). It is a snapshot taken when the object is created, so the values never change. Owned by the graphics
/// device, read it with `AGraphicsDevice::getGpuInfo()`.
class RIS_API AGpuInfo {
public:
    virtual ~AGpuInfo() = default;

    /// The name of the GPU (OpenGL ES: `GL_RENDERER`). Empty if the driver does not report it.
    virtual const std::string& getName() const noexcept = 0;

    /// The vendor of the GPU (OpenGL ES: `GL_VENDOR`). Empty if the driver does not report it.
    virtual const std::string& getVendor() const noexcept = 0;

    /// The version of the GPU API (OpenGL ES: `GL_VERSION`). Empty if the driver does not report it.
    virtual const std::string& getApiVersion() const noexcept = 0;

    /// The rendering API used by the graphics device, for example OpenGL ES.
    virtual RenderingBackend getApi() const noexcept = 0;

    /// The API below the one in `getApi()`, for example the one ANGLE translates to (Direct3D 11, Metal, Vulkan, ...).
    /// `Default` when it is not known: ANGLE does not report it through GL. GlesGpuInfo detects OpenGLES from the GL
    /// strings (`GL_VERSION` or `GL_RENDERER` contain "OpenGL ES"); the detection of the other APIs is still a TODO.
    virtual RenderingBackend getUnderlyingBackend() const noexcept = 0;

    /// The compressed texture formats the GPU can use, to choose a format for compressed textures: one representative
    /// format per supported family, Astc4x4Rgba (ASTC), Bc3RgbaUnorm (S3TC), Etc2Rgba8Unorm (ETC2) and Bc7RgbaUnorm
    /// (BC), in this order. PVRTC has no TextureFormat yet.
    virtual const std::vector<TextureFormat>& getSupportedCompressedTextureFormats() const noexcept = 0;

    AGpuInfo(const AGpuInfo&) = delete;
    AGpuInfo& operator=(const AGpuInfo&) = delete;

protected:
    AGpuInfo() = default;
};

} // namespace ris
