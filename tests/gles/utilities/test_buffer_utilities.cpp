// Tests of buffer_utilities (need a display).
#include <catch2/catch_test_macros.hpp>

#include <vector>

#include <ris/gles/gles.hpp>

#include "../gles_test_utils.hpp"

namespace gles = ris::gles;
namespace bu = ris::gles::buffer_utilities;
using F = ris::BufferUsageFlags;

TEST_CASE("buffer utilities create, update and delete buffers") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();

    SECTION("vertex buffers") {
        const std::vector<float> data(12, 1.0f);
        gles::GlUint vb = bu::create_vertex_buffer(data, F::Vertex | F::CopyDst, "vb");
        REQUIRE(vb != 0);
        REQUIRE_NOTHROW(bu::update_vertex_buffer(vb, std::vector<float>{2, 3}, 40));   // last two floats
        REQUIRE_NOTHROW(bu::update_vertex_buffer(vb, std::vector<float>{2, 3}));
        REQUIRE_THROWS_AS(bu::update_vertex_buffer(vb, std::span<const float>{}), ris::Error);
        REQUIRE_THROWS_AS(bu::update_vertex_buffer(0, data), ris::Error);
        bu::delete_buffer(vb);
        REQUIRE(vb == 0);
        REQUIRE_NOTHROW(bu::delete_buffer(vb));   // deleting 0 does nothing

        gles::GlUint empty = bu::create_vertex_buffer(64, F::Vertex);
        REQUIRE(empty != 0);
        bu::delete_buffer(empty);
        REQUIRE_THROWS_AS(bu::create_vertex_buffer(std::span<const float>{}, F::Vertex), ris::Error);
        REQUIRE_THROWS_AS(bu::create_vertex_buffer(0, F::Vertex), ris::Error);
    }
    SECTION("index buffers") {
        const std::vector<uint16_t> narrow = {0, 1, 2};
        const std::vector<uint32_t> wide = {0, 1, 2, 3};
        gles::GlUint a = bu::create_index_buffer(narrow, F::Index);
        gles::GlUint b = bu::create_index_buffer(wide, F::Index | F::CopyDst);
        REQUIRE(a != 0);
        REQUIRE(b != 0);
        REQUIRE(a != b);
        REQUIRE_NOTHROW(bu::update_buffer(ris::BufferType::Index, b, 4, wide.data(), 8));
        bu::delete_buffer(a);
        bu::delete_buffer(b);
    }
    SECTION("uniform buffers") {
        const std::vector<float> data = {1, 2, 3, 4};
        gles::GlUint ub = bu::create_uniform_buffer(data, F::Uniform | F::CopyDst, "ub");
        REQUIRE(ub != 0);
        REQUIRE_NOTHROW(bu::update_buffer(ris::BufferType::Uniform, ub, 0, data.data(), 16));
        bu::delete_buffer(ub);
        gles::GlUint sized = bu::create_uniform_buffer(32, F::Uniform);
        REQUIRE(sized != 0);
        bu::delete_buffer(sized);
    }
    SECTION("creating an index buffer keeps the element array binding of the vertex array") {
        // The framework creates buffers while vertex arrays exist: an index buffer must be filled through
        // a target that does not detach the index buffer of the current vertex array.
        const std::vector<uint16_t> idx = {0, 1, 2};
        const std::vector<float> vertices = {-1, -1, 3, -1, -1, 3};
        ris::gles::GlesVertexBuffer vb(vertices, 8);
        ris::gles::GlesIndexBuffer index(std::vector<uint16_t>{0, 1, 2});
        ris::VertexBufferLayout layout;
        layout.array_stride = 8;
        layout.attributes = {{ris::VertexFormat::Float32x2, 0, 0}};
        ris::VertexArray vao(ris::gles::make_vertex_array_desc(vb, layout, &index));
        ris::Shader shader(R"(#version 300 es
            layout(location = 0) in vec2 p;
            void main() { gl_Position = vec4(p, 0.0, 1.0); }
        )",
                           R"(#version 300 es
            precision mediump float;
            out vec4 frag;
            void main() { frag = vec4(1.0, 0.0, 0.0, 1.0); }
        )");
        shader.use();
        gles::GlUint other = bu::create_index_buffer(idx, F::Index);   // after the vertex array exists
        ris::RenderTarget target(8, 8, false);
        target.bind();
        ris::gfx::clear(0, 0, 0, 1, ris::ClearFlags::Color);
        vao.draw(ris::Topology::Triangles, 0, 3);
        REQUIRE(ris_test::read_pixel(4, 4)[0] == 255);
        ris::RenderTarget::unbind();
        bu::delete_buffer(other);
    }
}
