// Tests of VertexFormat, VertexAttribute and the graphics API neutral VertexBufferLayout (no display needed).
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
    REQUIRE_NOTHROW(l.validate());

    SECTION("attributes must fit the stride") {
        ris::VertexBufferLayout bad;
        bad.array_stride = 16;
        bad.attributes = {{ris::VertexFormat::Float32x4, 4, 0}};
        REQUIRE_THROWS_AS(bad.validate(), ris::Error);
        bad.attributes = {{ris::VertexFormat::Float32x4, 0, 0}};
        REQUIRE_NOTHROW(bad.validate());
        bad.attributes = {{ris::VertexFormat::Float32x4, 20, 0}};   // offset behind the stride
        REQUIRE_THROWS_AS(bad.validate(), ris::Error);
    }
}
