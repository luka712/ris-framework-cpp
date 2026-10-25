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
    // TODO: doc comment
        virtual ~AGraphicsDeviceFeatures() = default;

    // TODO: doc comment
        [[nodiscard]] virtual bool supportsTextureCompressionS3TC() const = 0;

    // TODO: doc comment
        [[nodiscard]] virtual bool supportsTextureCompressionBC() const = 0;

    // TODO: doc comment
        [[nodiscard]] virtual bool supportsTextureCompressionETC2() const = 0;

    // TODO: doc comment
        [[nodiscard]] virtual bool supportsTextureCompressionASTC() const = 0;

    // TODO: doc comment
        [[nodiscard]] virtual bool supportsTextureCompressionPVRTC() const = 0;

        // TODO: doc comment
        AGraphicsDeviceFeatures(const AGraphicsDeviceFeatures&) = delete;

    // TODO: doc comment
        AGraphicsDeviceFeatures& operator=(const AGraphicsDeviceFeatures&) = delete;

    protected:
        AGraphicsDeviceFeatures() = default;
};

} // namespace ris

#endif //RIS_FRAMEWORK_A_GRAPHICS_DEVICE_FEATURES_HPP
