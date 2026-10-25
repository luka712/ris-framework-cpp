// Port of the `unlit_mesh` example of ris-framework-ts: an OBJ cube with one unlit material per face,
// drawn with a perspective camera and a scene.
#include <iostream>

#include "../common/framework_args.hpp"

int main(int argc, char** argv) {
    try {
        ris::FrameworkConfiguration config;
        config.title = "ris sample: unlit_mesh";
        sample::apply_args(config, argc, argv);

        ris::Framework framework(config);

        // The scene must be destroyed before the framework, so it is declared after it.
        ris::Scene scene;
        std::shared_ptr<ris::PerspectiveCamera> camera;
        ris::Mesh* cube = nullptr;
        float angle = 0.0f;
        constexpr float rotation_speed = 0.6f;   // radians per second, independent of the frame rate

        framework.addOnLoadContentListener([&](ris::Framework&) {
            // Passing the scene makes the camera the scene's camera.
            const ris::Vec2 size = framework.renderer().back_buffer_size();
            camera = framework.cameraFactory().create_perspective_camera(
                {.fov = 60, .aspect_ratio = size.x / size.y, .near = 0.1f, .far = 100, .eye = {0, 0, 3}}, &scene);
            // One sub mesh per usemtl range (6 faces -> 6 sub meshes), each with an unlit material coloured by its Kd.
            cube = &framework.content().load_mesh(sample::asset("unlit_mesh", "cube.obj"), scene);
            std::cout << "Loaded cube.obj: " << cube->sub_meshes().size() << " submeshes, unlit materials.\n";
        });

        framework.addOnUpdateListener([&](ris::Framework&, const ris::GameTime& time) {
            if (!camera || !cube) return;
            angle += rotation_speed * time.delta_time_sec();
            cube->rotation = {angle * 0.7f, angle, 0};   // Euler radians (x, y, z), applied Y then X
            camera->aspect_ratio = framework.renderer().back_buffer_size().x / framework.renderer().back_buffer_size().y;
            camera->update(time);
            scene.update(time);
        });

        framework.addOnRenderListener([&](ris::Framework&) { scene.render(); });

        framework.renderer().setClearColor(ris::Color::light_pink());
        framework.initialize();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
