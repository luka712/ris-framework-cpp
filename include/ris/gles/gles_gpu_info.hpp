//
// Created by luka on 25.10.26.
//

#ifndef RIS_FRAMEWORK_GLES_GPU_INFO_HPP
#define RIS_FRAMEWORK_GLES_GPU_INFO_HPP

#include <string>
#include <vector>

#include "ris/rendering/a_gpu_info.hpp"
#include "ris/rendering/a_graphics_device_features.hpp"

namespace ris::gles {

/// The OpenGL ES implementation of AGpuInfo, like `WebGlGpuInfo` of ris-framework-ts. Reads `GL_RENDERER`,
/// `GL_VENDOR` and `GL_VERSION` (`glGetString`) and copies the strings once, in the
/// constructor, so it needs the window's GL context for the constructor only. The API is OpenGL ES. The underlying
/// backend is detected from the GL strings: `OpenGLES` when `GL_VERSION` or `GL_RENDERER` contain "OpenGL ES", else
/// `Default` (the API below ANGLE is not reported through GL, the detection of the other APIs is still a TODO). The compressed formats come from the features, in the order
/// documented in AGpuInfo::getSupportedCompressedTextureFormats(). The WebGL `WEBGL_debug_renderer_info` extension
/// (unmasked renderer and vendor) has no counterpart in OpenGL ES, so the names are the masked ones.
class RIS_API GlesGpuInfo : public AGpuInfo {
public:
    /// @param features the features of the graphics device, they decide the supported compressed formats.
    /// @throws std::logic_error without a window.
    explicit GlesGpuInfo(const AGraphicsDeviceFeatures& features);

    [[nodiscard]] const std::string& getName() const noexcept override { return m_name; }
    [[nodiscard]] const std::string& getVendor() const noexcept override { return m_vendor; }
    [[nodiscard]] const std::string& getApiVersion() const noexcept override { return m_apiVersion; }
    [[nodiscard]] RenderingBackend getApi() const noexcept override { return RenderingBackend::OpenGLES; }
    [[nodiscard]] RenderingBackend getUnderlyingBackend() const noexcept override { return m_renderingBackend; }
    [[nodiscard]] const std::vector<TextureFormat>& getSupportedCompressedTextureFormats() const noexcept override {
        return m_supportedCompressedTextureFormats;
    }

private:
    std::string m_name;
    std::string m_vendor;
    std::string m_apiVersion;
    std::vector<TextureFormat> m_supportedCompressedTextureFormats;
    RenderingBackend m_renderingBackend;
};

} // namespace ris::gles

#endif //RIS_FRAMEWORK_GLES_GPU_INFO_HPP
