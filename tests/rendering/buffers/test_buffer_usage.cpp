// Tests of BufferUsageFlags (ris/rendering/buffers/buffer_usage.hpp).
#include <catch2/catch_test_macros.hpp>

#include <ris/ris.hpp>

TEST_CASE("buffer usage flags") {
    using F = ris::BufferUsageFlags;
    const F f = F::Vertex | F::CopyDst;
    REQUIRE(ris::has_flag(f, F::Vertex));
    REQUIRE(ris::has_flag(f, F::CopyDst));
    REQUIRE_FALSE(ris::has_flag(f, F::Index));
    REQUIRE(static_cast<uint32_t>(f) == 33);   // same values as BufferUsage of ris-framework-ts
    REQUIRE(static_cast<uint32_t>(F::Storage) == 8);
}
