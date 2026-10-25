// Tests of GlesVertexBuffer on a real context (needs a display).
#include <catch2/catch_test_macros.hpp>

#include <vector>

#include <ris/ris.hpp>

#include "../../rendering/buffers/buffer_test_utils.hpp"

TEST_CASE("vertex buffer on a real context") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    using F = ris::BufferUsageFlags;

    SECTION("vertex buffer") {
        const std::vector<float> data(5 * 4, 0.5f);   // 4 vertices of float3 + float2
        ris::gles::GlesVertexBuffer vb(data, 20, F::Vertex | F::CopyDst, "quad");
        REQUIRE(vb.label() == "quad");
        REQUIRE(vb.vertex_count() == 4);
        REQUIRE(vb.byte_stride() == 20);
        REQUIRE(vb.byte_size() == 80);
        REQUIRE(vb.gl_handle() != 0);
        REQUIRE(ris::has_flag(vb.usage(), F::Vertex));
        REQUIRE(ris::has_flag(vb.usage(), F::CopyDst));

        const std::vector<float> one(5, 1.0f);
        REQUIRE_NOTHROW(vb.update(one, 60));                             // last vertex
        REQUIRE_THROWS_AS(vb.update(one, 64), ris::Error);               // 4 bytes too far
        REQUIRE_THROWS_AS(vb.update(std::span<const float>{}, 0), ris::Error);   // null data

        ris::gles::GlesVertexBuffer moved(std::move(vb));
        REQUIRE(moved.vertex_count() == 4);
        REQUIRE(vb.gl_handle() == 0);   // NOLINT moved-from

        auto dynamic = ris::gles::GlesVertexBuffer::with_byte_size(40, 20);
        REQUIRE(dynamic.vertex_count() == 2);
        REQUIRE_NOTHROW(dynamic.update(one));
    }
    SECTION("vertex buffer without CopyDst cannot be updated") {
        const std::vector<float> data(5, 0.0f);
        ris::gles::GlesVertexBuffer vb(data, 20);
        REQUIRE_THROWS_AS(vb.update(data), ris::Error);
    }
}
