#include <catch2/catch_test_macros.hpp>

#include <array>
#include <vector>

#include <ris/utilities/align_utilities.hpp>

namespace au = ris::align_utilities;

TEST_CASE("align rounds up to a multiple") {
    REQUIRE(au::align(0, 4) == 0);
    REQUIRE(au::align(1, 4) == 4);
    REQUIRE(au::align(4, 4) == 4);
    REQUIRE(au::align(5, 4) == 8);
    REQUIRE(au::align(17, 16) == 32);
    REQUIRE(au::align(7, 5) == 10);   // not a power of two (ASTC 5x5 blocks)
    REQUIRE(au::align(13, 0) == 13);  // no alignment
    static_assert(au::align(9, 8) == 16);
}

TEST_CASE("align_array pads with zeros") {
    const std::array<float, 3> three = {1, 2, 3};
    const std::vector<float> padded = au::align_array<float>(three, 4);
    REQUIRE(padded == std::vector<float>{1, 2, 3, 0});

    const std::array<float, 4> four = {1, 2, 3, 4};
    REQUIRE(au::align_array<float>(four, 4) == std::vector<float>{1, 2, 3, 4});
    REQUIRE(au::align_array<float>(std::span<const float>{}, 4).empty());
}
