// Tests of BufferUsageFlags (ris/rendering/buffers/buffer_usage.hpp).
#include <catch2/catch_test_macros.hpp>

#include <ris/ris.hpp>

TEST_CASE("buffer usage flags") {
    const ris::BufferUsageFlags f = ris::BufferUsageFlags::Vertex | ris::BufferUsageFlags::CopyDst;
    REQUIRE(ris::has_flag(f, ris::BufferUsageFlags::Vertex));
    REQUIRE(ris::has_flag(f, ris::BufferUsageFlags::CopyDst));
    REQUIRE_FALSE(ris::has_flag(f, ris::BufferUsageFlags::Index));
    REQUIRE(static_cast<uint32_t>(f) == 33);   // same values as BufferUsage of ris-framework-ts
    REQUIRE(static_cast<uint32_t>(ris::BufferUsageFlags::Storage) == 8);
}
