#pragma once

#include <cstdint>
#include <string>

#include "core.hpp"
#include "data.hpp"

namespace ris {

/// Maximum lights the built-in Blinn-Phong material shades, same limits as ris-framework-ts.
inline constexpr uint32_t max_directional_lights = 3;
inline constexpr uint32_t max_point_lights = 5;

/// Returns a process wide unique light id.
RIS_API uint32_t next_light_id();

struct AmbientLightDescriptor {
    std::string name;
    Color color = Color::white();
    float intensity = 1.0f;
};

class RIS_API AmbientLight {
public:
    explicit AmbientLight(const AmbientLightDescriptor& d = {});
    const uint32_t id;
    std::string name;
    Color color;
    float intensity;
};

struct DirectionalLightDescriptor {
    std::string name;
    Color color = Color::white();
    float intensity = 1.0f;
    glm::vec3 direction{0, -1, 0};   ///< the direction the light travels in
};

class RIS_API DirectionalLight {
public:
    explicit DirectionalLight(const DirectionalLightDescriptor& d = {});
    const uint32_t id;
    std::string name;
    Color color;
    float intensity;
    glm::vec3 direction;
};

struct PointLightDescriptor {
    std::string name;
    Color color = Color::white();
    float intensity = 1.0f;
    glm::vec3 position{0, 0, 0};
    float constant = 1.0f;      ///< attenuation = 1 / (constant + linear * d + quadratic * d * d)
    float linear = 0.09f;
    float quadratic = 0.032f;
};

class RIS_API PointLight {
public:
    explicit PointLight(const PointLightDescriptor& d = {});
    const uint32_t id;
    std::string name;
    Color color;
    float intensity;
    glm::vec3 position;
    float constant, linear, quadratic;
};

} // namespace ris
