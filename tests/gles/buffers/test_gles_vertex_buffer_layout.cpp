// Tests of the OpenGL ES conversion of a VertexBufferLayout (no display needed).
#include <catch2/catch_test_macros.hpp>

#include <ris/ris.hpp>

TEST_CASE("layouts convert to OpenGL ES vertex attributes") {
    const auto l = ris::VertexBufferLayout::create_float3_float4_float2_float3();
    const auto attribs = ris::gles::vertex_buffer_layout::to_vertex_attribs(l, 2);
    REQUIRE(attribs.size() == 4);
    REQUIRE(attribs[1].location == 1);
    REQUIRE(attribs[1].components == 4);
    REQUIRE(attribs[1].offset == 12);
    REQUIRE(attribs[1].buffer_slot == 2);
    REQUIRE(attribs[1].divisor == 0);

    SECTION("a mat4 expands to four vec4 columns, instance step sets the divisor") {
        ris::VertexBufferLayout m;
        m.array_stride = 64;
        m.step_mode = ris::VertexStepMode::Instance;
        m.attributes = {{ris::VertexFormat::Float32x16, 0, 4}};
        const auto a = ris::gles::vertex_buffer_layout::to_vertex_attribs(m);
        REQUIRE(a.size() == 4);
        for (uint32_t i = 0; i < 4; ++i) {
            REQUIRE(a[i].location == 4 + i);
            REQUIRE(a[i].offset == 16 * i);
            REQUIRE(a[i].components == 4);
            REQUIRE(a[i].divisor == 1);
        }
    }
    SECTION("attributes must fit the stride") {
        ris::VertexBufferLayout bad;
        bad.array_stride = 16;
        bad.attributes = {{ris::VertexFormat::Float32x4, 4, 0}};
        REQUIRE_THROWS_AS(ris::gles::vertex_buffer_layout::to_vertex_attribs(bad), ris::Error);
    }
}
