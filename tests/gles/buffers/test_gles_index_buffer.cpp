// Tests of GlesIndexBuffer on a real context (needs a display).
#include <catch2/catch_test_macros.hpp>

#include <vector>

#include <ris/ris.hpp>

#include "../../rendering/buffers/buffer_test_utils.hpp"

TEST_CASE("index buffer on a real context") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;

    SECTION("index buffer picks the smallest type") {
        const std::vector<uint32_t> small = {0, 1, 2, 2, 1, 65535};
        ris::gles::GlesIndexBuffer a(small);
        REQUIRE(a.type() == ris::IndexBufferType::UInt16);
        REQUIRE(a.element_byte_size() == 2);
        REQUIRE(a.indices_count() == 6);
        REQUIRE(a.byte_size() == 12);
        REQUIRE(a.index_type() == ris::IndexType::UInt16);
        REQUIRE(ris::has_flag(a.usage(), ris::BufferUsageFlags::Index));

        const std::vector<uint32_t> big = {0, 1, 65536};
        ris::gles::GlesIndexBuffer b(big, "big");
        REQUIRE(b.type() == ris::IndexBufferType::UInt32);
        REQUIRE(b.element_byte_size() == 4);
        REQUIRE(b.indices_count() == 3);
        REQUIRE(b.label() == "big");

        auto forced = ris::gles::GlesIndexBuffer::create_uint32(small);
        REQUIRE(forced.type() == ris::IndexBufferType::UInt32);
        REQUIRE(forced.byte_size() == 24);

        const std::vector<uint16_t> u16 = {0, 1, 2};
        REQUIRE(ris::gles::GlesIndexBuffer(u16).type() == ris::IndexBufferType::UInt16);
    }
}
