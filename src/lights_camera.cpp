#include "ris/camera.hpp"
#include "ris/lights.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>

#include "ris/framework.hpp"
#include "ris/scene.hpp"

namespace ris {

// ---- lights ----------------------------------------------------------------------------------

uint32_t next_light_id() {
    static std::atomic<uint32_t> counter{1};
    return counter.fetch_add(1);
}

AmbientLight::AmbientLight(const AmbientLightDescriptor& d)
    : id(next_light_id()), name(d.name), color(d.color), intensity(d.intensity) {}

DirectionalLight::DirectionalLight(const DirectionalLightDescriptor& d)
    : id(next_light_id()), name(d.name), color(d.color), intensity(d.intensity), direction(d.direction) {}

PointLight::PointLight(const PointLightDescriptor& d)
    : id(next_light_id()),
      name(d.name),
      color(d.color),
      intensity(d.intensity),
      position(d.position),
      constant(d.constant),
      linear(d.linear),
      quadratic(d.quadratic) {}

// ---- cameras ---------------------------------------------------------------------------------

Camera::~Camera() = default;

PerspectiveCamera::PerspectiveCamera(const PerspectiveCameraDescriptor& d)
    : eye(d.eye),
      target(d.target),
      up(d.up),
      field_of_view(glm::radians(d.fov)),
      aspect_ratio(d.aspect_ratio),
      near_plane(d.near),
      far_plane(d.far) {
    update_matrices();
}

void PerspectiveCamera::update_matrices() {
    projection_ = glm::perspective(field_of_view, aspect_ratio, near_plane, far_plane);
    view_ = glm::lookAt(world_position(), target, up);
    projection_view_ = projection_ * view_;
}

void PerspectiveCamera::update(const GameTime&) { update_matrices(); }

OrthographicCamera::OrthographicCamera(float l, float r, float t, float b, float n, float f)
    : left(l), right(r), top(t), bottom(b), near_plane(n), far_plane(f) {
    update_matrices();
}

void OrthographicCamera::update_matrices() {
    projection_ = glm::ortho(left, right, bottom, top, near_plane, far_plane);
    view_ = Mat4(1.0f);
    projection_view_ = projection_ * view_;
}

void OrthographicCamera::update(const GameTime&) { update_matrices(); }

OrbitCamera::OrbitCamera(std::shared_ptr<PerspectiveCamera> core, const AInputManager& input)
    : core_(std::move(core)), input_(input) {
    core_->eye = {0, 0, -3};
    core_->target = {0, 0, 0};
    sync_from_core();
}

void OrbitCamera::sync_from_core() {
    const Vec3 dir = glm::normalize(core_->target - core_->eye);
    const float y = std::clamp(dir.y, -1.0f, 1.0f);
    pitch_ = -std::clamp(std::asin(y), pitch_clamp.x, pitch_clamp.y);
    yaw_ = -std::atan2(dir.x, dir.z);
}

void OrbitCamera::update(const GameTime& time) {
    const MouseState& mouse = input_.mouse_state();
    bool moved = false;
    if (mouse.is_button_down(orbit_button) && (mouse.dx() != 0 || mouse.dy() != 0)) {
        yaw_ -= mouse.dx() * sensitivity;
        pitch_ += mouse.dy() * sensitivity;
        moved = true;
    }
    float radius = glm::length(core_->target - core_->eye);
    if (mouse.scroll_wheel_delta.y != 0) {
        const float step = std::min(scroll_speed * 0.5f, max_scroll_speed);
        radius = std::max(0.1f, radius - mouse.scroll_wheel_delta.y * step);
        moved = true;
    }
    if (moved) {
        pitch_ = std::clamp(pitch_, pitch_clamp.x, pitch_clamp.y);
        const float cp = std::cos(pitch_);
        core_->eye = core_->target + Vec3(radius * cp * std::sin(yaw_), radius * std::sin(pitch_), -radius * cp * std::cos(yaw_));
    }
    core_->update(time);
}

std::shared_ptr<PerspectiveCamera> CameraFactory::create_perspective_camera(const PerspectiveCameraDescriptor& d,
                                                                            Scene* scene) {
    auto camera = std::make_shared<PerspectiveCamera>(d);
    if (scene) scene->set_camera(camera);
    return camera;
}

std::shared_ptr<OrthographicCamera> CameraFactory::create_orthographic_camera(float left, float right, float top,
                                                                              float bottom, float near_plane,
                                                                              float far_plane) {
    return std::make_shared<OrthographicCamera>(left, right, top, bottom, near_plane, far_plane);
}

std::shared_ptr<OrthographicCamera> CameraFactory::create_default_orthographic_camera(Vec2 origin) {
    const Vec2 size = framework_.renderer().back_buffer_size();
    return std::make_shared<OrthographicCamera>(origin.x, origin.x + size.x, origin.y, origin.y + size.y, -1.0f, 1.0f);
}

std::shared_ptr<OrbitCamera> CameraFactory::create_orbit_camera(float fov, float aspect_ratio, float near_plane,
                                                                float far_plane) {
    PerspectiveCameraDescriptor d;
    d.fov = fov;
    d.aspect_ratio = aspect_ratio;
    d.near = near_plane;
    d.far = far_plane;
    return std::make_shared<OrbitCamera>(std::make_shared<PerspectiveCamera>(d), framework_.input_manager());
}

} // namespace ris
