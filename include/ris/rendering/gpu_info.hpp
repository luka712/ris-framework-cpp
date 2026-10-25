#pragma once

#include <string>
#include <vector>

#include "ris/rendering/rendering_backend.hpp"
#include "ris/rendering/texture/texture_format.hpp"

namespace ris {

/// Information about the GPU, like `IGPUInfo` of ris-framework-ts. Plain data, filled by
/// `AGraphicsDevice::initialize()` and read with `AGraphicsDevice::getGpuInfo()`.
struct GpuInfo {
    /// The name of the GPU (OpenGL ES: `GL_RENDERER`).
    std::string name;
    /// The vendor of the GPU (OpenGL ES: `GL_VENDOR`).
    std::string vendor;
    /// The version of the GPU API (OpenGL ES: `GL_VERSION`).
    std::string apiVersion;
    /// The version of the shading language (OpenGL ES: `GL_SHADING_LANGUAGE_VERSION`). Not in ris-framework-ts.
    std::string shadingLanguageVersion;
    /// The rendering API used by the graphics device, for example OpenGL ES.
    RenderingBackend api = RenderingBackend::Default;
    /// The API below the one in `api`, for example the one ANGLE translates to (Direct3D 11, Metal, Vulkan, ...).
    /// `Default` when it is not known, which is the case for OpenGL ES: ANGLE does not report it through GL.
    RenderingBackend underlyingBackend = RenderingBackend::Default;
    /// The compressed texture formats the GPU can use, to choose a format for compressed textures. One
    /// representative format per supported family: Astc4x4Rgba (ASTC), Bc3RgbaUnorm (S3TC), Etc2Rgba8Unorm
    /// (ETC2) and Bc7RgbaUnorm (BC). PVRTC has no TextureFormat yet.
    std::vector<TextureFormat> supportedCompressedTextureFormats;
};

} // namespace ris
