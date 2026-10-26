// Port of the `blinn_phong_mesh` example of ris-framework-ts: an OBJ cube with Blinn-Phong materials,
// a pulsing ambient light, one directional light and two coloured point lights orbiting the cube.
#include <cmath>
#include <iostream>

#include "../common/framework_args.hpp"

int main(int argc, char** argv) {
    try {
        FrameworkConfiguration config = ris_createFrameworkConfiguration();
        config.title = "ris sample: blinn_phong_mesh";
        sample::apply_args(config, argc, argv);

        ris::Framework framework(config);

        ris::Scene scene;
        std::shared_ptr<ris::PerspectiveCamera> camera;
        ris::Mesh* cube = nullptr;
        float angle = 0.0f, orbit_angle = 0.0f;
        constexpr float rotation_speed = 0.6f;   // radians per second
        constexpr float orbit_speed = 1.2f;      // radians per second
        constexpr float pi = 3.14159265358979f;

        framework.addOnLoadContentListener([&](ris::Framework&) {
            const glm::vec2 size = framework.renderer().back_buffer_size();
            // Passing the scene makes the camera the scene's camera. The eye z is flipped by the look-at
            // (ris-framework-ts convention), so -3 views the cube from world z = +3.
            camera = framework.cameraFactory().create_perspective_camera({.aspect_ratio = size.x / size.y, .eye = {0, 0, -3}}, &scene);
            scene.active_ambient_light().color = ris::Color(0.4f, 0.4f, 0.4f);

            scene.create_directional_light({.name = "sun", .color = ris::Color(1, 1, 1), .intensity = 0.8f, .direction = {-0.4f, -0.6f, 1}});
            // Two coloured point lights orbiting the cube; positions are updated every frame.
            scene.create_point_light({.name = "warm", .color = ris::Color(1.0f, 0.5f, 0.2f), .intensity = 1.2f, .linear = 0.35f, .quadratic = 0.44f});
            scene.create_point_light({.name = "cool", .color = ris::Color(0.2f, 0.5f, 1.0f), .intensity = 1.2f, .linear = 0.35f, .quadratic = 0.44f});

            // One sub mesh per usemtl range with Blinn-Phong materials from the .mtl (Kd, Ka falling back to Kd, Ks, Ns).
            cube = &framework.content().load_blinn_phong_mesh(sample::asset("blinn_phong_mesh", "cube.obj"), scene);
            std::cout << "Loaded cube.obj: " << cube->sub_meshes().size() << " submeshes, Blinn-Phong materials, ambient light.\n";
        });

        framework.addOnUpdateListener([&](ris::Framework&, const ris::GameTime& time) {
            if (!camera || !cube) return;

            // Slowly pulse the ambient light.
            scene.active_ambient_light().intensity = 0.75f + 0.5f * std::sin(time.elapsed_time_sec());

            // Orbit the point lights around the cube (radius 1.6), opposite each other, tilted on y.
            orbit_angle += orbit_speed * time.delta_time_sec();
            for (size_t i = 0; i < scene.point_lights().size(); ++i) {
                const float a = orbit_angle + static_cast<float>(i) * pi;
                scene.point_lights()[i]->position = {1.6f * std::cos(a), 0.6f * std::sin(a * 2), 1.6f * std::sin(a)};
            }
            scene.update(time);

            angle += rotation_speed * time.delta_time_sec();
            cube->rotation = {angle * 0.7f, angle, 0};

            const glm::vec2 size = framework.renderer().back_buffer_size();
            camera->aspect_ratio = size.x / size.y;
            camera->update(time);
        });

        framework.addOnRenderListener([&](ris::Framework&) { scene.render(); });

        framework.renderer().setClearColor(ris::Color::corner_flower_blue());
        sample::apply_run_args(framework, argc, argv);
        framework.initialize();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
