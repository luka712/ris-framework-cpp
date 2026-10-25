// Tests of GlesUniformBuffer on a real context (needs a display).
#include <catch2/catch_test_macros.hpp>

#include <vector>

#include <ris/ris.hpp>

#include "../../rendering/buffers/buffer_test_utils.hpp"

TEST_CASE("uniform buffer on a real context") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    using F = ris::BufferUsageFlags;

    SECTION("uniform buffer is padded to 16 bytes") {
        const std::vector<float> data(5, 1.0f);   // 20 bytes
        ris::gles::GlesUniformBuffer ub(data);
        REQUIRE(ub.byte_size() == 32);
        REQUIRE(ris::has_flag(ub.usage(), F::Uniform));
        REQUIRE_NOTHROW(ub.update(ris::Vec4(1, 2, 3, 4), 16));
        REQUIRE_NOTHROW(ub.update(data, 12));
        REQUIRE_THROWS_AS(ub.update(ris::Vec4(1), 20), std::out_of_range);

        auto sized = ris::gles::GlesUniformBuffer::with_byte_size(65);
        REQUIRE(sized.byte_size() == 80);
        REQUIRE_NOTHROW(sized.update(ris::Mat4(1.0f), 16));
        REQUIRE_THROWS_AS(sized.update(ris::Mat4(1.0f), 32), std::out_of_range);

        ris::gles::GlesUniformBuffer fixed(data, F::Uniform);   // no CopyDst
        REQUIRE_THROWS_AS(fixed.update(data), std::invalid_argument);
    }
}
