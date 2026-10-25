#pragma once

#include <memory>
#include <vector>

#include "camera.hpp"
#include "core.hpp"
#include "lights.hpp"
#include "mesh.hpp"

namespace ris {

/// Holds the camera, lights and meshes of a scene, like `Scene` of ris-framework-ts.
///
/// The scene owns its meshes; lights and cameras are shared. Destroy it before the Framework.
class RIS_API Scene {
public:
    Scene();
    ~Scene();
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    std::shared_ptr<Camera> camera() const { return camera_; }
    void set_camera(std::shared_ptr<Camera> camera) { camera_ = std::move(camera); }

    AmbientLight& active_ambient_light() { return *active_ambient_; }
    const AmbientLight& active_ambient_light() const { return *active_ambient_; }
    /// Adds the light if needed and makes it the lit one.
    void set_active_ambient_light(std::shared_ptr<AmbientLight> light);
    const std::vector<std::shared_ptr<AmbientLight>>& ambient_lights() const { return ambient_lights_; }
    const std::vector<std::shared_ptr<DirectionalLight>>& directional_lights() const { return directional_lights_; }
    const std::vector<std::shared_ptr<PointLight>>& point_lights() const { return point_lights_; }
    const std::vector<std::unique_ptr<Mesh>>& meshes() const { return meshes_; }

    /// Creates a light and adds it. Throws when `max_directional_lights` / `max_point_lights` is exceeded.
    std::shared_ptr<DirectionalLight> create_directional_light(const DirectionalLightDescriptor& d = {});
    std::shared_ptr<PointLight> create_point_light(const PointLightDescriptor& d = {});
    std::shared_ptr<AmbientLight> create_ambient_light(const AmbientLightDescriptor& d = {});

    void add(std::shared_ptr<AmbientLight> light);
    void add(std::shared_ptr<DirectionalLight> light);
    void add(std::shared_ptr<PointLight> light);
    Mesh& add(std::unique_ptr<Mesh> mesh);
    bool remove(const AmbientLight& light);
    bool remove(const DirectionalLight& light);
    bool remove(const PointLight& light);
    bool remove(const Mesh& mesh);

    /// Per frame hook, kept for parity with the TS API (nothing to upload with plain uniforms).
    void update(const GameTime& time);
    /// Draws all visible meshes with the scene camera. Throws if there is no camera.
    void render();

private:
    std::shared_ptr<Camera> camera_;
    std::vector<std::shared_ptr<AmbientLight>> ambient_lights_;
    std::vector<std::shared_ptr<DirectionalLight>> directional_lights_;
    std::vector<std::shared_ptr<PointLight>> point_lights_;
    std::vector<std::unique_ptr<Mesh>> meshes_;
    std::shared_ptr<AmbientLight> active_ambient_;
};

} // namespace ris
