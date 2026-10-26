#pragma once

#include <memory>

#include "core.hpp"
#include "input.hpp"
#include "math.hpp"
#include "ris/time/game_time.hpp"

namespace ris {

class Framework;
class Scene;

/// Base of all cameras. Mirrors `ICamera` of ris-framework-ts.
///
/// Matrices are recomputed in `update()`, which the application calls once per frame (as in the TS
/// framework) before `Scene::render()`.
class RIS_API Camera {
public:
    virtual ~Camera();
    virtual const glm::mat4& projection_matrix() const = 0;
    virtual const glm::mat4& view_matrix() const = 0;
    virtual const glm::mat4& projection_view_matrix() const = 0;
    /// The position used for specular lighting, in world space.
    virtual glm::vec3 world_position() const = 0;
    virtual void update(const GameTime& time) = 0;
};

struct PerspectiveCameraDescriptor {
    float fov = 60.0f;            ///< vertical field of view in degrees
    float aspect_ratio = 1.0f;
    float near = 0.01f;
    float far = 100.0f;
    glm::vec3 eye{0, 0, 3};
    glm::vec3 target{0, 0, 0};
    glm::vec3 up{0, 1, 0};
};

/// Perspective camera with ris-framework-ts conventions: the view matrix is a look-at from
/// `(eye.x, eye.y, -eye.z)`, so an eye of `(0, 0, 3)` looks at the origin from world z = -3.
class RIS_API PerspectiveCamera : public Camera {
public:
    explicit PerspectiveCamera(const PerspectiveCameraDescriptor& d = {});

    glm::vec3 eye{0, 0, 3};
    glm::vec3 target{0, 0, 0};
    glm::vec3 up{0, 1, 0};
    float field_of_view;          ///< radians
    float aspect_ratio;
    float near_plane;
    float far_plane;

    glm::vec3 direction() const { return target - eye; }

    const glm::mat4& projection_matrix() const override { return projection_; }
    const glm::mat4& view_matrix() const override { return view_; }
    const glm::mat4& projection_view_matrix() const override { return projection_view_; }
    glm::vec3 world_position() const override { return {eye.x, eye.y, -eye.z}; }
    void update(const GameTime& time) override;

private:
    void update_matrices();
    glm::mat4 projection_{1.0f}, view_{1.0f}, projection_view_{1.0f};
};

/// Orthographic camera, `top` and `bottom` follow screen space when `top < bottom` (y down).
class RIS_API OrthographicCamera : public Camera {
public:
    OrthographicCamera(float left, float right, float top, float bottom, float near_plane, float far_plane);

    float left, right, top, bottom, near_plane, far_plane;

    const glm::mat4& projection_matrix() const override { return projection_; }
    const glm::mat4& view_matrix() const override { return view_; }
    const glm::mat4& projection_view_matrix() const override { return projection_view_; }
    glm::vec3 world_position() const override { return {0, 0, 1}; }
    void update(const GameTime& time) override;

private:
    void update_matrices();
    glm::mat4 projection_{1.0f}, view_{1.0f}, projection_view_{1.0f};
};

/// Orbits around `target` while the orbit button is held, mouse wheel zooms. Wraps a PerspectiveCamera
/// like `OrbitCamera` of ris-framework-ts (mouse input only, no touch in 0.1.0).
class RIS_API OrbitCamera : public Camera {
public:
    OrbitCamera(std::shared_ptr<PerspectiveCamera> core, const InputManager& input);

    float sensitivity = 0.005f;   ///< radians per pixel of mouse movement
    float scroll_speed = 1.0f;
    float max_scroll_speed = 50.0f;
    glm::vec2 pitch_clamp{-1.55334f, 1.55334f};
    MouseButton orbit_button = MouseButton::Left;

    PerspectiveCamera& core_camera() { return *core_; }
    const PerspectiveCamera& core_camera() const { return *core_; }
    glm::vec3& eye() { return core_->eye; }
    glm::vec3& target() { return core_->target; }
    /// Recomputes yaw and pitch from the current eye and target. Call after changing them.
    void sync_from_core();

    const glm::mat4& projection_matrix() const override { return core_->projection_matrix(); }
    const glm::mat4& view_matrix() const override { return core_->view_matrix(); }
    const glm::mat4& projection_view_matrix() const override { return core_->projection_view_matrix(); }
    glm::vec3 world_position() const override { return core_->world_position(); }
    void update(const GameTime& time) override;

private:
    std::shared_ptr<PerspectiveCamera> core_;
    const InputManager& input_;
    float yaw_ = 0, pitch_ = 0;
};

/// Creates cameras, like `CameraFactory` of ris-framework-ts.
class RIS_API CameraFactory {
public:
    explicit CameraFactory(Framework& framework) : framework_(framework) {}

    /// When a scene is given the camera becomes the scene's camera.
    std::shared_ptr<PerspectiveCamera> create_perspective_camera(const PerspectiveCameraDescriptor& d = {},
                                                                 Scene* scene = nullptr);
    std::shared_ptr<OrthographicCamera> create_orthographic_camera(float left, float right, float top, float bottom,
                                                                   float near_plane, float far_plane);
    /// Pixel space camera for the current back buffer size, (0, 0) at the top left, y pointing down.
    std::shared_ptr<OrthographicCamera> create_default_orthographic_camera(glm::vec2 origin = {0, 0});
    std::shared_ptr<OrbitCamera> create_orbit_camera(float fov = 60.0f, float aspect_ratio = 1.0f, float near_plane = 0.01f,
                                                     float far_plane = 100.0f);

private:
    Framework& framework_;
};

} // namespace ris
