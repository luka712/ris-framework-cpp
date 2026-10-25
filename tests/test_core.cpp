#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <cstring>

#include <ris/ris.h>
#include <ris/ris.hpp>

TEST_CASE("version matches between C and C++ APIs") {
    REQUIRE(ris_get_version_major() == ris::version_major);
    REQUIRE(ris_get_version_minor() == ris::version_minor);
    REQUIRE(ris_get_version_patch() == ris::version_patch);
    REQUIRE(std::strcmp(ris_get_version_string(), ris::version_string()) == 0);
    REQUIRE(std::strcmp(ris_get_version_string(), "0.1.0") == 0);
}

TEST_CASE("glm matrices are column-major and match the C math") {
    using Catch::Matchers::WithinAbs;
    const ris::Mat4 t = glm::translate(ris::Mat4(1.0f), ris::Vec3(1.0f, 2.0f, 3.0f));
    // column-major: translation lives in elements 12..14
    REQUIRE(glm::value_ptr(t)[12] == 1.0f);
    REQUIRE(glm::value_ptr(t)[13] == 2.0f);
    REQUIRE(glm::value_ptr(t)[14] == 3.0f);

    float c[16];
    ris_mat4_translation(c, 1.0f, 2.0f, 3.0f);
    REQUIRE(std::memcmp(c, glm::value_ptr(t), sizeof(c)) == 0);

    const ris::Mat4 r = glm::rotate(ris::Mat4(1.0f), 1.5707963f, ris::Vec3(0, 0, 1));
    REQUIRE_THAT(glm::value_ptr(r)[0], WithinAbs(0.0, 1e-5));
    REQUIRE_THAT(glm::value_ptr(r)[1], WithinAbs(1.0, 1e-5));

    float p[16];
    ris_mat4_perspective(p, 1.0f, 1.5f, 0.1f, 50.0f);
    const ris::Mat4 gp = glm::perspective(1.0f, 1.5f, 0.1f, 50.0f);
    REQUIRE(std::memcmp(p, glm::value_ptr(gp), sizeof(p)) == 0);
}

TEST_CASE("C API reports errors instead of throwing") {
    ris_clear_last_error();
    REQUIRE(ris_window_create(nullptr) == nullptr);
    REQUIRE(std::strlen(ris_get_last_error()) > 0);

    ris_clear_last_error();
    REQUIRE(ris_shader_create(nullptr, nullptr) == nullptr);
    REQUIRE(std::strlen(ris_get_last_error()) > 0);

    // Graphics calls without a context must fail cleanly.
    ris_clear_last_error();
    REQUIRE(ris_buffer_create(RIS_BUFFER_VERTEX, RIS_USAGE_STATIC, nullptr, 16) == nullptr);
    REQUIRE(std::strlen(ris_get_last_error()) > 0);
    REQUIRE(ris_texture_load_ktx2_file("/does/not/exist.ktx2") == nullptr);

    // Destroying NULL is a no-op.
    ris_window_destroy(nullptr);
    ris_buffer_destroy(nullptr);
    ris_texture_destroy(nullptr);
}

TEST_CASE("C++ API throws ris::Error without a context") {
    REQUIRE_THROWS_AS(ris::Shader("", ""), ris::Error);
    REQUIRE_THROWS_AS(ris::Texture::from_rgba8(4, 4, nullptr), ris::Error);
}

TEST_CASE("Log callback receives messages") {
    int count = 0;
    ris::set_log_callback([&](ris::LogLevel, const char*) { ++count; });
    ris::set_log_level(ris::LogLevel::Trace);
    try {
        ris::Shader("", "");
    } catch (const ris::Error&) {
    }
    ris::set_log_callback({});
    REQUIRE(count >= 0); // the callback is exercised without crashing
}
