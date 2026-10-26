// Tests of shader_utilities and program_utilities (need a display).
#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <string>

#include <ris/gles/gles.hpp>

#include "../gles_test_utils.hpp"

namespace gles = ris::gles;

namespace {
const std::string kVertex = R"(#version 300 es
    void main() { gl_Position = vec4(0.0, 0.0, 0.0, 1.0); }
)";
const std::string kFragment = R"(#version 300 es
    precision mediump float;
    out vec4 frag;
    void main() { frag = vec4(1.0); }
)";

std::string error_message(const std::function<void()>& fn) {
    try {
        fn();
    } catch (const std::runtime_error& e) {
        return e.what();
    }
    return {};
}
} // namespace

TEST_CASE("shader and program utilities") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;

    SECTION("compile and link") {
        uint32_t vs = gles::shader_utilities::create_vertex_shader(kVertex);
        uint32_t fs = gles::shader_utilities::create(kFragment, ris::ShaderStage::Fragment);
        REQUIRE(vs != 0);
        REQUIRE(fs != 0);
        uint32_t program = gles::program_utilities::create(vs, fs);
        REQUIRE(program != 0);
        gles::shader_utilities::delete_shader(vs);
        gles::shader_utilities::delete_shader(fs);
        REQUIRE(vs == 0);
        gles::program_utilities::delete_program(program);
        REQUIRE(program == 0);
        REQUIRE_NOTHROW(gles::error_utilities::check_error("shader utilities"));
    }
    SECTION("compile errors carry the stage and the log") {
        const std::string vertex = error_message([] { gles::shader_utilities::create_vertex_shader("#version 300 es\nnot glsl"); });
        REQUIRE(vertex.find("vertex shader compile error") != std::string::npos);
        const std::string fragment = error_message([] { gles::shader_utilities::create_fragment_shader("#version 300 es\nnot glsl"); });
        REQUIRE(fragment.find("fragment shader compile error") != std::string::npos);
    }
    SECTION("link errors") {
        // the vertex shader writes no varying that the fragment shader reads
        uint32_t vs = gles::shader_utilities::create_vertex_shader(kVertex);
        uint32_t fs = gles::shader_utilities::create_fragment_shader(R"(#version 300 es
            precision mediump float;
            in vec2 v_missing;
            out vec4 frag;
            void main() { frag = vec4(v_missing, 0.0, 1.0); }
        )");
        const std::string msg = error_message([&] { gles::program_utilities::create(vs, fs); });
        REQUIRE(msg.find("shader link error") != std::string::npos);
        gles::shader_utilities::delete_shader(vs);
        gles::shader_utilities::delete_shader(fs);
    }
    SECTION("only vertex and fragment stages compile") {
        REQUIRE_THROWS_AS(gles::shader_utilities::create(kVertex, ris::ShaderStage::Compute), std::domain_error);
        REQUIRE_THROWS_AS(gles::shader_utilities::create(kVertex, ris::ShaderStage::None), std::domain_error);
    }
}
