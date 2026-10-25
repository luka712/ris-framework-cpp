// Tests of the high level (ris-framework-ts like) C++ API. The CPU only parts need no display.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdlib>

#include <ris/ris.hpp>

using Catch::Approx;

namespace {
bool display_available() {
#if defined(__linux__)
    return std::getenv("DISPLAY") != nullptr || std::getenv("WAYLAND_DISPLAY") != nullptr;
#else
    return true;
#endif
}
} // namespace

TEST_CASE("Color and Rect") {
    const ris::Color c = ris::Color::corner_flower_blue();
    REQUIRE(c.r == Approx(0.39f));
    REQUIRE(c.a == 1.0f);
    REQUIRE(ris::Color::white() == ris::Color(1, 1, 1, 1));

    const ris::Rect r(10, 20, 30, 40);
    REQUIRE(r.right() == 40);
    REQUIRE(r.bottom() == 60);
    REQUIRE(r.contains(10, 20));
    REQUIRE_FALSE(r.contains(41, 30));
    REQUIRE(r.intersects({39, 59, 10, 10}));
    REQUIRE_FALSE(r.intersects({40, 60, 10, 10}));
}

TEST_CASE("perspective camera follows the ris-framework-ts conventions") {
    ris::PerspectiveCamera camera({.aspect_ratio = 2.0f, .eye = {0, 0, 3}});
    // the look-at flips the eye z
    REQUIRE(camera.world_position() == ris::Vec3(0, 0, -3));
    // the origin is 3 units in front of the camera, i.e. at view z = -3
    const ris::Vec4 origin = camera.view_matrix() * ris::Vec4(0, 0, 0, 1);
    REQUIRE(origin.z == Approx(-3.0f));
    camera.eye = {0, 2, 4};
    camera.update({});
    REQUIRE(camera.world_position() == ris::Vec3(0, 2, -4));
    REQUIRE(camera.field_of_view == Approx(glm::radians(60.0f)));
}

TEST_CASE("scene lights") {
    ris::Scene scene;
    REQUIRE(scene.ambient_lights().size() == 1);
    scene.active_ambient_light().color = {0.4f, 0.4f, 0.4f};

    auto sun = scene.create_directional_light({.name = "sun"});
    auto other = scene.create_point_light({.name = "p"});
    REQUIRE(sun->id != other->id);
    REQUIRE(sun->name == "sun");

    for (uint32_t i = 1; i < ris::max_directional_lights; ++i) scene.create_directional_light();
    REQUIRE_THROWS_AS(scene.create_directional_light(), ris::Error);

    REQUIRE(scene.remove(*sun));
    REQUIRE_FALSE(scene.remove(*sun));
    REQUIRE_NOTHROW(scene.create_directional_light());

    REQUIRE_THROWS_AS(scene.render(), ris::Error);   // no camera
}

TEST_CASE("geometry builder") {
    const ris::Geometry cube = ris::GeometryBuilder::create_cube(2.0f);
    REQUIRE(cube.vertex_count() == 24);
    REQUIRE(cube.indices.size() == 36);
    REQUIRE(cube.normals.size() == 72);
    const ris::Geometry quad = ris::GeometryBuilder::create_quad(2, 4);
    REQUIRE(quad.vertex_count() == 4);
}

TEST_CASE("framework runs a frame loop and listeners") {
    if (!display_available()) SKIP("no display available");
    ris::FrameworkConfiguration config;
    config.hidden = true;
    config.back_buffer_size = {64, 64};
    config.max_frames = 5;

    ris::Framework framework(config);
    ris::Scene scene;
    int load = 0, init = 0, updates = 0, renders = 0, removed_calls = 0;
    double last_delta = 0;

    framework.add_on_load_content_listener([&] { ++load; });
    framework.add_on_initialized_listener([&] { ++init; });
    framework.add_on_update_listener([&](const ris::GameTime& t) { ++updates; last_delta = t.delta_time_ms; });
    const auto id = framework.add_on_render_listener([&] { ++removed_calls; });
    REQUIRE(framework.remove_on_render_listener(id));
    framework.add_on_render_listener([&] { ++renders; });

    // a unit cube in front of a camera, drawn with both built-in materials
    framework.camera_factory().create_perspective_camera({.eye = {0, 0, 3}}, &scene);
    auto cube = ris::GeometryBuilder::create_cube();
    framework.mesh_factory().create(scene, cube, framework.material_factory().create_unlit_material());
    framework.mesh_factory().create(scene, cube, framework.material_factory().create_blinn_phong_material()).position = {0, 1, 0};
    scene.create_point_light({.position = {1, 1, -2}});
    framework.add_on_render_listener([&] { scene.render(); });

    framework.initialize();
    REQUIRE(load == 1);
    REQUIRE(init == 1);
    REQUIRE(updates == 5);
    REQUIRE(renders == 5);
    REQUIRE(removed_calls == 0);
    REQUIRE(last_delta == Approx(1000.0 / 60.0));
}
