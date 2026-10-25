// Tests of the high level (ris-framework-ts like) C++ API. The CPU only parts need no display.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <string>
#include <vector>

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

    framework.addOnLoadContentListener([&] { ++load; });
    framework.addOnInitializedListener([&] { ++init; });
    framework.addOnUpdateListener([&](const ris::GameTime& t) { ++updates; last_delta = t.delta_time_ms; });
    const auto id = framework.addOnRenderListener([&] { ++removed_calls; });
    REQUIRE(framework.removeOnRenderListener(id));
    framework.addOnRenderListener([&] { ++renders; });

    // a unit cube in front of a camera, drawn with both built-in materials
    framework.cameraFactory().create_perspective_camera({.eye = {0, 0, 3}}, &scene);
    auto cube = ris::GeometryBuilder::create_cube();
    framework.meshFactory().create(scene, cube, framework.materialFactory().create_unlit_material());
    framework.meshFactory().create(scene, cube, framework.materialFactory().create_blinn_phong_material()).position = {0, 1, 0};
    scene.create_point_light({.position = {1, 1, -2}});
    framework.addOnRenderListener([&] { scene.render(); });

    framework.initialize();
    REQUIRE(load == 1);
    REQUIRE(init == 1);
    REQUIRE(updates == 5);
    REQUIRE(renders == 5);
    REQUIRE(removed_calls == 0);
    REQUIRE(last_delta == Approx(1000.0 / 60.0));
}

TEST_CASE("framework drives the time manager: deterministic frames and fixed update steps") {
    if (!display_available()) SKIP("no display available");
    ris::FrameworkConfiguration config;
    config.hidden = true;
    config.back_buffer_size = {64, 64};
    config.max_frames = 6;
    config.fixed_time_step_ms = 1000.0 / 120.0;   // two fixed steps per deterministic 60 Hz frame
    config.target_frames_per_second = 30;         // ignored in deterministic mode

    ris::Framework framework(config);
    REQUIRE(framework.timeManager().deterministic_step_ms() == Approx(1000.0 / 60.0));
    REQUIRE(framework.timeManager().fixed_time_step_ms() == Approx(1000.0 / 120.0));
    REQUIRE(framework.timeManager().target_frames_per_second() == 30);

    int fixed = 0, updates = 0;
    double last_fixed_elapsed = 0, last_elapsed = 0;
    framework.addOnFixedUpdateListener([&](const ris::GameTime& t) {
        ++fixed;
        REQUIRE(t.delta_time_ms == Approx(1000.0 / 120.0));
        last_fixed_elapsed = t.elapsed_time_ms;
    });
    const auto removed = framework.addOnFixedUpdateListener([&](const ris::GameTime&) { FAIL("removed"); });
    REQUIRE(framework.removeOnFixedUpdateListener(removed));
    REQUIRE_FALSE(framework.removeOnFixedUpdateListener(removed));
    framework.addOnUpdateListener([&](const ris::GameTime& t) {
        ++updates;
        REQUIRE(t.frame_number == static_cast<uint64_t>(updates));
        last_elapsed = t.elapsed_time_ms;
    });
    framework.initialize();

    REQUIRE(updates == 6);
    REQUIRE(fixed >= 11);   // 2 per frame, one may be lost to rounding of the accumulator
    REQUIRE(fixed <= 12);
    REQUIRE(last_fixed_elapsed == Approx(1000.0 / 120.0 * fixed));
    REQUIRE(last_elapsed == Approx(5 * 1000.0 / 60.0));
}

TEST_CASE("framework listeners: ids are 1, 2, 3 ... per kind and shared by all frameworks, either callback form") {
    if (!display_available()) SKIP("no display available");
    ris::FrameworkConfiguration config;
    config.hidden = true;
    config.back_buffer_size = {32, 32};
    config.max_frames = 3;

    uint32_t a = 0, c = 0;
    {
        ris::Framework first(config);
        a = first.addOnUpdateListener([](const ris::GameTime&) {});
        c = first.addOnUpdateListener([](const ris::GameTime&) {});
        REQUIRE(a != 0);
        REQUIRE(a < c);
        REQUIRE(first.removeOnUpdateListener(a));
        REQUIRE_FALSE(first.removeOnUpdateListener(a));   // already removed
        REQUIRE_FALSE(first.removeOnUpdateListener(0));
    }

    // only one window can exist at a time, so the second framework is created after the first is gone;
    // the id counter is static: its ids continue after the ones of the first framework
    ris::Framework first(config);
    const auto b = first.addOnUpdateListener([](ris::AFramework&, const ris::GameTime&) {});
    REQUIRE(b > c);
    REQUIRE_FALSE(first.removeOnUpdateListener(c));   // belonged to the destroyed framework
    REQUIRE(first.removeOnUpdateListener(b));

    // every kind has its own counter
    const auto l1 = first.addOnLoadContentListener([] {});
    const auto l2 = first.addOnLoadContentListener([](ris::AFramework&) {});
    const auto i1 = first.addOnInitializedListener([] {});
    const auto f1 = first.addOnFixedUpdateListener([](const ris::GameTime&) {});
    const auto r1 = first.addOnRenderListener([] {});
    REQUIRE(l2 > l1);
    REQUIRE(first.removeOnLoadContentListener(l1));
    REQUIRE(first.removeOnLoadContentListener(l2));
    REQUIRE(first.removeOnInitializedListener(i1));
    REQUIRE(first.removeOnFixedUpdateListener(f1));
    REQUIRE(first.removeOnRenderListener(r1));
}

TEST_CASE("framework listeners run in registration order and may add or remove listeners while running") {
    if (!display_available()) SKIP("no display available");
    ris::FrameworkConfiguration config;
    config.hidden = true;
    config.back_buffer_size = {32, 32};
    config.max_frames = 3;

    ris::Framework framework(config);
    std::vector<std::string> log;
    uint32_t victim = 0;
    bool added = false;

    // the framework is passed to the callbacks that ask for it
    framework.addOnLoadContentListener([&](ris::AFramework& f) {
        REQUIRE(&f == &framework);
        log.push_back("load");
    });
    framework.addOnInitializedListener([&] { log.push_back("init"); });
    framework.addOnUpdateListener([&](ris::AFramework& f, const ris::GameTime& t) {
        REQUIRE(&f == &framework);
        log.push_back("u1#" + std::to_string(t.frame_number));
        f.removeOnUpdateListener(victim);   // removed before it ran: never called
        if (!added) {                       // added now: first runs on the next frame
            added = true;
            f.addOnUpdateListener([&](const ris::GameTime& t2) { log.push_back("late#" + std::to_string(t2.frame_number)); });
        }
    });
    victim = framework.addOnUpdateListener([&](const ris::GameTime&) { log.push_back("victim"); });
    framework.addOnUpdateListener([&](const ris::GameTime& t) { log.push_back("u3#" + std::to_string(t.frame_number)); });
    framework.addOnRenderListener([&] { log.push_back("render"); });

    framework.initialize();

    const std::vector<std::string> expected{
        "load", "init",
        "u1#1", "u3#1", "render",
        "u1#2", "u3#2", "late#2", "render",
        "u1#3", "u3#3", "late#3", "render"};
    REQUIRE(log == expected);
}
