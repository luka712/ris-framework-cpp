// Tests of BufferFactory on a real context (needs a display).
#include <catch2/catch_test_macros.hpp>

#include <vector>

#include <ris/ris.hpp>

#include "buffer_test_utils.hpp"

TEST_CASE("buffer factory on a real context") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    using F = ris::BufferUsageFlags;

    SECTION("factory") {
        ris::BufferFactory factory;
        const std::vector<float> data(6, 0.0f);
        const std::vector<uint32_t> idx = {0, 1, 2};
        auto vb = factory.create_vertex_buffer(data, 12, F::Vertex, "v");
        auto ib = factory.create_index_buffer(idx, "i");
        auto ub = factory.create_uniform_buffer(data, F::Uniform | F::CopyDst, "u");
        REQUIRE(vb->vertex_count() == 2);
        REQUIRE(ib->indices_count() == 3);
        REQUIRE(ub->byte_size() == 32);
        REQUIRE(ub->label() == "u");
    }
}
