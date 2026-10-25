// Tests of the graphics API neutral argument checks shared by all buffer backends (no display needed).
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <ris/ris.hpp>

TEST_CASE("usage flags are resolved per buffer kind") {
    namespace bv = ris::buffer_validation;
    using F = ris::BufferUsageFlags;
    const F unsupported = F::Storage | F::MapRead;

    REQUIRE(bv::resolve_usage(F::CopyDst, F::Vertex, "T", unsupported, "API") == (F::Vertex | F::CopyDst));
    REQUIRE(bv::resolve_usage(F::None, F::Index, "T", unsupported, "API") == F::Index);
    REQUIRE_THROWS_AS(([&] { bv::resolve_usage(F::Index, F::Vertex, "T", unsupported, "API"); })(), std::invalid_argument);
    REQUIRE_THROWS_AS(([&] { bv::resolve_usage(F::Storage, F::Vertex, "T", unsupported, "API"); })(), std::domain_error);
    // a backend that supports storage buffers simply passes fewer unsupported flags
    REQUIRE(bv::resolve_usage(F::Storage, F::Vertex, "T", F::None, "API") == (F::Vertex | F::Storage));

    try {
        bv::resolve_usage(F::MapRead, F::Uniform, "UniformBuffer", unsupported, "Some API 1.0");
        FAIL("expected an error");
    } catch (const std::exception& e) {
        REQUIRE(std::string(e.what()).find("Some API 1.0") != std::string::npos);
    }
}

TEST_CASE("vertex, index and uniform sizes are validated") {
    namespace bv = ris::buffer_validation;
    using F = ris::BufferUsageFlags;
    const F unsupported = F::Storage | F::MapRead;

    REQUIRE(bv::resolve_vertex_buffer(48, 12, F::Vertex, unsupported, "API") == F::Vertex);
    REQUIRE_THROWS_AS(([&] { bv::resolve_vertex_buffer(48, 0, F::Vertex, unsupported, "API"); })(), std::invalid_argument);
    REQUIRE_THROWS_AS(([&] { bv::resolve_vertex_buffer(0, 12, F::Vertex, unsupported, "API"); })(), std::invalid_argument);
    REQUIRE_THROWS_AS(([&] { bv::resolve_vertex_buffer(48, 20, F::Vertex, unsupported, "API"); })(), std::invalid_argument);

    REQUIRE(bv::resolve_index_buffer(6) == F::Index);
    REQUIRE_THROWS_AS(([] { bv::resolve_index_buffer(0); })(), std::invalid_argument);

    REQUIRE(bv::padded_uniform_byte_size(1) == 16);
    REQUIRE(bv::padded_uniform_byte_size(16) == 16);
    REQUIRE(bv::padded_uniform_byte_size(65) == 80);
    REQUIRE_THROWS_AS(([] { bv::padded_uniform_byte_size(0); })(), std::invalid_argument);

    const std::vector<float> five(5, 1.0f);
    const std::vector<float> padded = bv::pad_uniform_data(five);
    REQUIRE(padded.size() == 8);
    REQUIRE(padded[4] == 1.0f);
    REQUIRE(padded[5] == 0.0f);
    REQUIRE_THROWS_AS(([] { bv::pad_uniform_data({}); })(), std::invalid_argument);
}

TEST_CASE("index type selection picks 16 bit when everything fits") {
    namespace bv = ris::buffer_validation;
    const std::vector<uint32_t> small = {0, 1, 65535};
    const std::vector<uint32_t> big = {0, 65536};
    REQUIRE(bv::select_index_type(small) == ris::IndexBufferType::UInt16);
    REQUIRE(bv::select_index_type(big) == ris::IndexBufferType::UInt32);
    REQUIRE(bv::narrow_indices(small) == std::vector<uint16_t>{0, 1, 65535});
    REQUIRE(bv::narrow_indices(big).empty());
}

TEST_CASE("update checks") {
    namespace bv = ris::buffer_validation;
    using F = ris::BufferUsageFlags;
    const float value = 1.0f;

    REQUIRE_NOTHROW(bv::check_update_allowed(F::Vertex | F::CopyDst, "T"));
    REQUIRE_THROWS_AS(([&] { bv::check_update_allowed(F::Vertex, "T"); })(), std::invalid_argument);

    REQUIRE_NOTHROW(bv::check_update_range(16, 12, &value, 4, "T"));
    REQUIRE_THROWS_AS(([&] { bv::check_update_range(16, 13, &value, 4, "T"); })(), std::out_of_range);
    REQUIRE_THROWS_AS(([&] { bv::check_update_range(16, 0, nullptr, 4, "T"); })(), std::invalid_argument);

    auto layout = ris::VertexBufferLayout::create_float3_float2();
    REQUIRE_NOTHROW(bv::check_layout_stride(layout, 20, "T"));
    REQUIRE_THROWS_AS(([&] { bv::check_layout_stride(layout, 24, "T"); })(), std::invalid_argument);
}
