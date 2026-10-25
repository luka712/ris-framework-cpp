#include "ris/gles/gles_gpu_info.hpp"

#include <SDL3/SDL_platform.h>

#include "internal.hpp"
#include "ris/gles/utilities/gles_utilities.hpp"

using namespace ris::detail;

namespace ris::gles {
    namespace {
    } // namespace

    GlesGpuInfo::GlesGpuInfo(const AGraphicsDeviceFeatures &features) {
        require_context(); // throws std::logic_error without a window, glGetString needs the context
        m_name = GlesUtilities::context().glString(GL_RENDERER);
        m_vendor = GlesUtilities::context().glString(GL_VENDOR);
        m_apiVersion = GlesUtilities::context().glString(GL_VERSION);
        m_renderingBackend = RenderingBackend::Default;

        // The same order as WebGlGpuInfo.
        if (features.supportsTextureCompressionASTC())
            m_supportedCompressedTextureFormats.push_back(
                TextureFormat::Astc4x4Rgba);
        if (features.supportsTextureCompressionS3TC())
            m_supportedCompressedTextureFormats.push_back(
                TextureFormat::Bc3RgbaUnorm);
        if (features.supportsTextureCompressionETC2())
            m_supportedCompressedTextureFormats.push_back(
                TextureFormat::Etc2Rgba8Unorm);
        if (features.supportsTextureCompressionBC())
            m_supportedCompressedTextureFormats.push_back(
                TextureFormat::Bc7RgbaUnorm);
        // TODO: PVRTC, there is no TextureFormat for it yet.

        if (m_name.find("Direct3D11") != std::string_view::npos ||
            m_name.find("D3D11") != std::string_view::npos) {
            // TODO:
        } else if (m_name.find("Vulkan") != std::string_view::npos) {
            // TODO:
        } else if (m_name.find("Metal") != std::string_view::npos) {
            // TODO:
        } else if (m_name.find("OpenGL ES") != std::string_view::npos
                   || m_apiVersion.find("OpenGL ES") != std::string_view::npos) {
            m_renderingBackend = RenderingBackend::OpenGLES;
        } else if (m_name.find("OpenGL") != std::string_view::npos) {
            // TODO:
        }
    }
} // namespace ris::gles
