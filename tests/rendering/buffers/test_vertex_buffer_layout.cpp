// Tests of VertexFormat, VertexAttribute and VertexBufferLayout (no display needed).
#include <catch2/catch_test_macros.hpp>

#include <ris/ris.hpp>

TEST_CASE("vertex format sizes and layouts") {
    REQUIRE(ris::vertex_format_byte_size(ris::VertexFormat::Float32) == 4);
    REQUIRE(ris::vertex_format_byte_size(ris::VertexFormat::Float32x3) == 12);
    REQUIRE(ris::vertex_format_byte_size(ris::VertexFormat::Float32x16) == 64);

    const auto l = ris::VertexBufferLayout::create_float3_float4_float2_float3();
    REQUIRE(l.array_stride == 48);
    REQUIRE(l.attributes.size() == 4);
    REQUIRE(l.attributes[3].offset == 36);
    REQUIRE(ris::VertexBufferLayout::create_float3_float2().array_stride == 20);
    REQUIRE(ris::VertexBufferLayout::create_float3_float4_float2().array_stride == 36);

    const auto attribs = l.to_vertex_attribs(2);
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
        const auto a = m.to_vertex_attribs();
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
        REQUIRE_THROWS_AS(bad.to_vertex_attribs(), ris::Error);
    }
}
