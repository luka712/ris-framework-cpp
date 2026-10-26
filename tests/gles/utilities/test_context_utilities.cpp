// Tests of the error, extension and debug label utilities (need a display).
#include <catch2/catch_test_macros.hpp>

#include <ris/gles/gles.hpp>

#include "../gles_test_utils.hpp"

namespace gles = ris::gles;

TEST_CASE("error utilities name the GL errors") {
    REQUIRE(std::string(gles::error_utilities::error_name(0)) == "GL_NO_ERROR");
    REQUIRE(std::string(gles::error_utilities::error_name(0x0500)) == "GL_INVALID_ENUM");
    REQUIRE(std::string(gles::error_utilities::error_name(0x0501)) == "GL_INVALID_VALUE");
    REQUIRE(std::string(gles::error_utilities::error_name(0x0502)) == "GL_INVALID_OPERATION");
    REQUIRE(std::string(gles::error_utilities::error_name(0x0506)) == "GL_INVALID_FRAMEBUFFER_OPERATION");
    REQUIRE(std::string(gles::error_utilities::error_name(0x0505)) == "GL_OUT_OF_MEMORY");
    REQUIRE(std::string(gles::error_utilities::error_name(0x1234)) == "unknown");
}

TEST_CASE("utilities need a window") {
    // No Window exists here: every call that talks to GL throws instead of crashing.
    REQUIRE_THROWS_AS(gles::error_utilities::check_error("test"), std::logic_error);
    REQUIRE_THROWS_AS(GlesUtilities::context().isExtensionSupported("GL_KHR_debug"), std::logic_error);
    REQUIRE_THROWS_AS(GlesUtilities::buffer().createVertexBuffer(16, ris::BufferUsageFlags::Vertex), std::logic_error);
    REQUIRE_THROWS_AS(GlesUtilities::texture().createTexture2D(4, 4, static_cast<const void*>(nullptr)), std::logic_error);
    REQUIRE_THROWS_AS(gles::shader_utilities::create_vertex_shader("x"), std::logic_error);
    // deleting needs no window
    uint32_t none = 7;
    GlesUtilities::buffer().deleteBuffer(none);
    REQUIRE(none == 0);
}

TEST_CASE("error, extension and debug label utilities on a real context") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;

    SECTION("check_error") {
        REQUIRE_NOTHROW(gles::error_utilities::check_error("nothing happened"));
        REQUIRE_NOTHROW(gles::error_utilities::clear_errors());
    }
    SECTION("extensions") {
        REQUIRE_FALSE(GlesUtilities::context().isExtensionSupported(""));
        REQUIRE_FALSE(GlesUtilities::context().isExtensionSupported("GL_RIS_no_such_extension"));
        // a prefix of a real extension name must not match
        REQUIRE_FALSE(GlesUtilities::context().isExtensionSupported("GL_KHR_debu"));
    }
    SECTION("debug labels never fail") {
        uint32_t buffer = GlesUtilities::buffer().createVertexBuffer(16, ris::BufferUsageFlags::Vertex, "labelled");
        REQUIRE_NOTHROW(gles::debug_label_utilities::is_supported());
        REQUIRE_NOTHROW(gles::debug_label_utilities::set_label(gles::debug_label_utilities::ObjectKind::Buffer, buffer, "again"));
        REQUIRE_NOTHROW(gles::debug_label_utilities::set_label(gles::debug_label_utilities::ObjectKind::Buffer, buffer, ""));
        REQUIRE_NOTHROW(gles::debug_label_utilities::set_label(gles::debug_label_utilities::ObjectKind::Buffer, 0, "none"));
        REQUIRE_NOTHROW(gles::error_utilities::check_error("labels"));
        GlesUtilities::buffer().deleteBuffer(buffer);
    }
}
