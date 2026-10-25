//
// Created by luka on 25.10.26.
//

#ifndef RIS_FRAMEWORK_A_GRAPHICS_DEVICE_FEATURES_HPP
#define RIS_FRAMEWORK_A_GRAPHICS_DEVICE_FEATURES_HPP

#include "ris/core.hpp"

namespace ris {

/// The features of this graphics device.
class RIS_API AGraphicsDeviceFeatures {
    public:
        virtual ~AGraphicsDeviceFeatures() = default;

        /// Supports S3TC / DXT (BC1–BC3) texture compression.
        [[nodiscard]] virtual bool supportsTextureCompressionS3TC() const = 0;

        /// Supports full Block Compression (BC1–BC7) textures. Includes S3TC (BC1–BC3) plus BC4–BC7.
        [[nodiscard]] virtual bool supportsTextureCompressionBC() const = 0;

        /// Supports ETC2 / EAC texture compression (OpenGL/ES and Vulkan standard).
        [[nodiscard]] virtual bool supportsTextureCompressionETC2() const = 0;

        /// Supports ASTC (Adaptive Scalable Texture Compression) — LDR and HDR profiles.
        [[nodiscard]] virtual bool supportsTextureCompressionASTC() const = 0;

        /// Supports PVRTC (PowerVR Texture Compression) 1 and 2.
        [[nodiscard]] virtual bool supportsTextureCompressionPVRTC() const = 0;

        AGraphicsDeviceFeatures(const AGraphicsDeviceFeatures&) = delete;
        AGraphicsDeviceFeatures& operator=(const AGraphicsDeviceFeatures&) = delete;

    protected:
        AGraphicsDeviceFeatures() = default;
};

} // namespace ris

#endif //RIS_FRAMEWORK_A_GRAPHICS_DEVICE_FEATURES_HPP
