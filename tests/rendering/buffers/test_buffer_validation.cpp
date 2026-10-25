// Argument validation of the typed buffers: errors are thrown before any GL call, so no display is needed.
#include <catch2/catch_test_macros.hpp>

#include <vector>

#include <ris/ris.hpp>

TEST_CASE("buffers validate their arguments before touching GL") {
    // No window exists here. Argument errors are InvalidArgument / Unsupported, not NotInitialized.
    const auto code_of = [](auto&& fn) {
        try {
            fn();
        } catch (const ris::Error& e) {
            return e.code();
        }
        return ris::Result::Ok;
    };
    const std::vector<float> floats(12, 1.0f);
    using F = ris::BufferUsageFlags;

    REQUIRE(code_of([&] { ris::VertexBuffer({}, 12); }) == ris::Result::InvalidArgument);
    REQUIRE(code_of([&] { ris::VertexBuffer(floats, 0); }) == ris::Result::InvalidArgument);
    REQUIRE(code_of([&] { ris::VertexBuffer(floats, 20); }) == ris::Result::InvalidArgument);   // 48 % 20 != 0
    REQUIRE(code_of([&] { ris::VertexBuffer(floats, 12, F::Vertex | F::Index); }) == ris::Result::InvalidArgument);
    REQUIRE(code_of([&] { ris::VertexBuffer(floats, 12, F::Vertex | F::Storage); }) == ris::Result::Unsupported);
    REQUIRE(code_of([&] { ris::IndexBuffer(std::span<const uint32_t>{}); }) == ris::Result::InvalidArgument);
    REQUIRE(code_of([&] { ris::UniformBuffer(std::span<const float>{}); }) == ris::Result::InvalidArgument);
    REQUIRE(code_of([&] { ris::UniformBuffer::with_byte_size(0); }) == ris::Result::InvalidArgument);
    REQUIRE(code_of([&] { ris::UniformBuffer(floats, F::Uniform | F::MapRead); }) == ris::Result::Unsupported);
    // valid arguments reach the context check
    REQUIRE(code_of([&] { ris::VertexBuffer(floats, 12); }) == ris::Result::NotInitialized);
    REQUIRE(code_of([&] { ris::UniformBuffer{floats}; }) == ris::Result::NotInitialized);

}
