// Argument validation of the typed buffers: errors are thrown before any GL call, so no display is needed.
#include <catch2/catch_test_macros.hpp>

#include <vector>

#include <ris/ris.hpp>

TEST_CASE("buffers validate their arguments before touching GL") {
    // No window exists here. Argument errors are std::invalid_argument / std::domain_error, not std::logic_error.
    const std::vector<float> floats(12, 1.0f);

    REQUIRE_THROWS_AS(([&] { ris::gles::GlesVertexBuffer({}, 12); })(), std::invalid_argument);
    REQUIRE_THROWS_AS(([&] { ris::gles::GlesVertexBuffer(floats, 0); })(), std::invalid_argument);
    REQUIRE_THROWS_AS(([&] { ris::gles::GlesVertexBuffer(floats, 20); })(), std::invalid_argument);   // 48 % 20 != 0
    REQUIRE_THROWS_AS(([&] { ris::gles::GlesVertexBuffer(floats, 12, ris::BufferUsageFlags::Vertex | ris::BufferUsageFlags::Index); })(), std::invalid_argument);
    REQUIRE_THROWS_AS(([&] { ris::gles::GlesVertexBuffer(floats, 12, ris::BufferUsageFlags::Vertex | ris::BufferUsageFlags::Storage); })(), std::domain_error);
    REQUIRE_THROWS_AS(([&] { ris::gles::GlesIndexBuffer(std::span<const uint32_t>{}); })(), std::invalid_argument);
    REQUIRE_THROWS_AS(([&] { ris::gles::GlesUniformBuffer(std::span<const float>{}); })(), std::invalid_argument);
    REQUIRE_THROWS_AS(([&] { ris::gles::GlesUniformBuffer::with_byte_size(0); })(), std::invalid_argument);
    REQUIRE_THROWS_AS(([&] { ris::gles::GlesUniformBuffer(floats, ris::BufferUsageFlags::Uniform | ris::BufferUsageFlags::MapRead); })(), std::domain_error);
    // valid arguments reach the context check
    REQUIRE_THROWS_AS(([&] { ris::gles::GlesVertexBuffer(floats, 12); })(), std::logic_error);
    REQUIRE_THROWS_AS(([&] { ris::gles::GlesUniformBuffer{floats}; })(), std::logic_error);

}
