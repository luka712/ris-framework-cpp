// Tests of make_vertex_array_desc on a real context (needs a display).
#include <catch2/catch_test_macros.hpp>

#include <span>
#include <vector>

#include <ris/ris.hpp>

#include "../../rendering/buffers/buffer_test_utils.hpp"

TEST_CASE("vertex array description checks the stride") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();

    const std::vector<float> data(10, 0.0f);
    ris::gles::GlesVertexBuffer vb(data, 20);
    REQUIRE_THROWS_AS(ris::gles::make_vertex_array_desc(vb, ris::VertexBufferLayout::create_float3_float4_float2()), std::invalid_argument);
}

TEST_CASE("typed buffers draw a quad with a uniform block") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();

    // two triangles covering the whole viewport: position (vec3) + uv (vec2)
    const std::vector<float> vertices = {
        -1, -1, 0, 0, 0,
         1, -1, 0, 1, 0,
         1,  1, 0, 1, 1,
        -1,  1, 0, 0, 1,
    };
    const std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};
    const auto layout = ris::VertexBufferLayout::create_float3_float2();

    ris::gles::GlesVertexBuffer vb(vertices, layout.array_stride);
    ris::gles::GlesIndexBuffer ib(indices);
    ris::gles::GlesUniformBuffer color(std::span<const float>(std::vector<float>{1, 0, 0, 1}));
    ris::VertexArray vao(ris::gles::make_vertex_array_desc(vb, layout, &ib));

    ris::Shader shader(R"(#version 300 es
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec2 a_uv;
        void main() { gl_Position = vec4(a_position, 1.0); }
    )",
                       R"(#version 300 es
        precision mediump float;
        layout(std140) uniform Color { vec4 color; };
        out vec4 frag;
        void main() { frag = color; }
    )");
    shader.use();
    shader.bind_uniform_block("Color", 0);
    color.bind_base(0);

    ris::RenderTarget target(16, 16, false);
    target.bind();

    const auto draw_and_read = [&] {
        ris::gfx::clear(0, 0, 0, 1, ris::ClearFlags::Color);
        vao.draw(ris::Topology::Triangles, 0, ib.indices_count());
        return ris::gfx::read_pixels(8, 8, 1, 1);
    };

    auto px = draw_and_read();
    REQUIRE(px[0] == 255);
    REQUIRE(px[1] == 0);
    REQUIRE(px[2] == 0);

    color.update(glm::vec4(0, 1, 0, 1));
    px = draw_and_read();
    REQUIRE(px[0] == 0);
    REQUIRE(px[1] == 255);

    ris::RenderTarget::unbind();
}
