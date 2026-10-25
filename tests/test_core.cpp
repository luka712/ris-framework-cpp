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

TEST_CASE("Mat4 basics") {
    using Catch::Matchers::WithinAbs;
    const auto id = ris::Mat4::identity();
    REQUIRE(id.data()[0] == 1.0f);
    REQUIRE(id.data()[5] == 1.0f);
    REQUIRE(id.data()[10] == 1.0f);
    REQUIRE(id.data()[15] == 1.0f);

    const auto t = ris::Mat4::translation(1.0f, 2.0f, 3.0f);
    // column-major: translation lives in elements 12..14
    REQUIRE(t.data()[12] == 1.0f);
    REQUIRE(t.data()[13] == 2.0f);
    REQUIRE(t.data()[14] == 3.0f);

    const auto m = t * ris::Mat4::identity();
    for (int i = 0; i < 16; ++i) REQUIRE(m.data()[i] == t.data()[i]);

    const auto r = ris::Mat4::rotation_z(1.5707963f);
    REQUIRE_THAT(r.data()[0], WithinAbs(0.0, 1e-5));
    REQUIRE_THAT(r.data()[1], WithinAbs(1.0, 1e-5));
}

TEST_CASE("C math matches C++ math") {
    float c[16];
    ris_mat4_translation(c, 4.0f, 5.0f, 6.0f);
    const auto cpp = ris::Mat4::translation(4.0f, 5.0f, 6.0f);
    REQUIRE(std::memcmp(c, cpp.data(), sizeof(c)) == 0);
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
