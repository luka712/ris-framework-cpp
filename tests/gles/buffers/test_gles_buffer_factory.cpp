// Tests of GlesBufferFactory and of the buffers used only through the abstract classes (needs a display).
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <vector>

#include <ris/ris.hpp>

#include "../../rendering/buffers/buffer_test_utils.hpp"

TEST_CASE("buffer factory on a real context") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;

    SECTION("the default factory creates the OpenGL ES buffers") {
        const std::unique_ptr<ris::ABufferFactory> factory = ris::create_default_buffer_factory();
        REQUIRE(dynamic_cast<ris::gles::GlesBufferFactory*>(factory.get()) != nullptr);

        const std::vector<float> data(6, 0.0f);
        const std::vector<uint32_t> idx = {0, 1, 2};
        std::unique_ptr<ris::AVertexBuffer> vb = factory->create_vertex_buffer(data, 12, ris::BufferUsageFlags::Vertex, "v");
        std::unique_ptr<ris::AIndexBuffer> ib = factory->create_index_buffer(idx, "i");
        std::unique_ptr<ris::AUniformBuffer> ub = factory->create_uniform_buffer(data, ris::BufferUsageFlags::Uniform | ris::BufferUsageFlags::CopyDst, "u");
        REQUIRE(vb->vertex_count() == 2);
        REQUIRE(vb->label() == "v");
        REQUIRE(ib->indices_count() == 3);
        REQUIRE(ib->type() == ris::IndexBufferType::UInt16);
        REQUIRE(ub->byte_size() == 32);
        REQUIRE(ub->label() == "u");
        REQUIRE(ris::has_flag(ub->usage(), ris::BufferUsageFlags::Uniform));

        // the native handles are only reachable on the concrete classes
        const auto* gl_vb = dynamic_cast<const ris::gles::GlesVertexBuffer*>(vb.get());
        REQUIRE(gl_vb != nullptr);
        REQUIRE(gl_vb->gl_handle() != 0);
        REQUIRE(dynamic_cast<const ris::gles::GlesIndexBuffer*>(ib.get()) != nullptr);
        REQUIRE(dynamic_cast<const ris::gles::GlesUniformBuffer*>(ub.get()) != nullptr);
    }

    SECTION("update and sized creation through the abstract classes") {
        ris::gles::GlesBufferFactory gles_factory;
        const ris::ABufferFactory& factory = gles_factory;

        auto vb = factory.create_vertex_buffer_with_byte_size(40, 20);
        REQUIRE(vb->vertex_count() == 2);
        const std::vector<float> one(5, 1.0f);
        REQUIRE_NOTHROW(vb->update(one, 20));
        REQUIRE_THROWS_AS(vb->update(one, 24), std::out_of_range);

        auto ub = factory.create_uniform_buffer_with_byte_size(65);
        REQUIRE(ub->byte_size() == 80);
        REQUIRE_NOTHROW(ub->update(glm::mat4(1.0f), 16));
        REQUIRE_NOTHROW(ub->update(glm::vec4(1), 0));
        REQUIRE_THROWS_AS(ub->update(glm::mat4(1.0f), 32), std::out_of_range);

        const std::vector<uint32_t> idx = {0, 1, 2};
        auto wide = factory.create_index_buffer_uint32(idx);
        REQUIRE(wide->type() == ris::IndexBufferType::UInt32);
        const std::vector<uint16_t> narrow = {0, 1, 2};
        REQUIRE(factory.create_index_buffer(narrow)->type() == ris::IndexBufferType::UInt16);
    }

    SECTION("a vertex array can be built from the abstract buffers") {
        const auto factory = ris::create_default_buffer_factory();
        const std::vector<float> vertices = {-1, -1, 0, 0, 0, 1, -1, 0, 1, 0, 0, 1, 0, 0.5f, 1};
        const std::vector<uint32_t> indices = {0, 1, 2};
        const auto layout = ris::VertexBufferLayout::create_float3_float2();
        auto vb = factory->create_vertex_buffer(vertices, layout.array_stride);
        auto ib = factory->create_index_buffer(indices);
        REQUIRE_NOTHROW(ris::VertexArray(ris::gles::make_vertex_array_desc(*vb, layout, ib.get())));
        REQUIRE_THROWS_AS(ris::gles::make_vertex_array_desc(*vb, ris::VertexBufferLayout::create_float3_float4_float2()), std::invalid_argument);
    }
}
