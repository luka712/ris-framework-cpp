// Tests of GlesBufferUtilities (need a display).
#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <vector>

#include <ris/gles/gles.hpp>

#include "../gles_test_utils.hpp"

namespace gles = ris::gles;

TEST_CASE("GlesBufferUtilities create, update and delete buffers") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;

    SECTION("vertex buffers") {
        const std::vector<float> data(12, 1.0f);
        uint32_t vb = GlesUtilities::buffer().createVertexBuffer(data, ris::BufferUsageFlags::Vertex | ris::BufferUsageFlags::CopyDst, "vb");
        REQUIRE(vb != 0);
        REQUIRE_NOTHROW(GlesUtilities::buffer().updateVertexBuffer(vb, std::vector<float>{2, 3}, 40));   // last two floats
        REQUIRE_NOTHROW(GlesUtilities::buffer().updateVertexBuffer(vb, std::vector<float>{2, 3}));
        REQUIRE_THROWS_AS(GlesUtilities::buffer().updateVertexBuffer(vb, std::span<const float>{}), std::invalid_argument);
        REQUIRE_THROWS_AS(GlesUtilities::buffer().updateVertexBuffer(0, data), std::invalid_argument);
        GlesUtilities::buffer().deleteBuffer(vb);
        REQUIRE(vb == 0);
        REQUIRE_NOTHROW(GlesUtilities::buffer().deleteBuffer(vb));   // deleting 0 does nothing

        uint32_t empty = GlesUtilities::buffer().createVertexBuffer(64, ris::BufferUsageFlags::Vertex);
        REQUIRE(empty != 0);
        GlesUtilities::buffer().deleteBuffer(empty);
        REQUIRE_THROWS_AS(GlesUtilities::buffer().createVertexBuffer(std::span<const float>{}, ris::BufferUsageFlags::Vertex), std::invalid_argument);
        REQUIRE_THROWS_AS(GlesUtilities::buffer().createVertexBuffer(0, ris::BufferUsageFlags::Vertex), std::invalid_argument);
    }
    SECTION("index buffers") {
        const std::vector<uint16_t> narrow = {0, 1, 2};
        const std::vector<uint32_t> wide = {0, 1, 2, 3};
        uint32_t a = GlesUtilities::buffer().createIndexBuffer(narrow, ris::BufferUsageFlags::Index);
        uint32_t b = GlesUtilities::buffer().createIndexBuffer(wide, ris::BufferUsageFlags::Index | ris::BufferUsageFlags::CopyDst);
        REQUIRE(a != 0);
        REQUIRE(b != 0);
        REQUIRE(a != b);
        REQUIRE_NOTHROW(GlesUtilities::buffer().updateBuffer(ris::BufferType::Index, b, 4, wide.data(), 8));
        GlesUtilities::buffer().deleteBuffer(a);
        GlesUtilities::buffer().deleteBuffer(b);
    }
    SECTION("uniform buffers") {
        const std::vector<float> data = {1, 2, 3, 4};
        uint32_t ub = GlesUtilities::buffer().createUniformBuffer(data, ris::BufferUsageFlags::Uniform | ris::BufferUsageFlags::CopyDst, "ub");
        REQUIRE(ub != 0);
        REQUIRE_NOTHROW(GlesUtilities::buffer().updateBuffer(ris::BufferType::Uniform, ub, 0, data.data(), 16));
        GlesUtilities::buffer().deleteBuffer(ub);
        uint32_t sized = GlesUtilities::buffer().createUniformBuffer(32, ris::BufferUsageFlags::Uniform);
        REQUIRE(sized != 0);
        GlesUtilities::buffer().deleteBuffer(sized);
    }
    SECTION("creating an index buffer keeps the element array binding of the vertex array") {
        // The framework creates buffers while vertex arrays exist: an index buffer must be filled through
        // a target that does not detach the index buffer of the current vertex array.
        const std::vector<uint16_t> idx = {0, 1, 2};
        const std::vector<float> vertices = {-1, -1, 3, -1, -1, 3};
        ris::gles::GlesVertexBuffer vb(vertices, 8);
        ris::gles::GlesIndexBuffer index(std::vector<uint16_t>{0, 1, 2});
        ris::VertexBufferLayout layout;
        layout.array_stride = 8;
        layout.attributes = {{ris::VertexFormat::Float32x2, 0, 0}};
        ris::VertexArray vao(ris::gles::make_vertex_array_desc(vb, layout, &index));
        ris::Shader shader(R"(#version 300 es
            layout(location = 0) in vec2 p;
            void main() { gl_Position = vec4(p, 0.0, 1.0); }
        )",
                           R"(#version 300 es
            precision mediump float;
            out vec4 frag;
            void main() { frag = vec4(1.0, 0.0, 0.0, 1.0); }
        )");
        shader.use();
        uint32_t other = GlesUtilities::buffer().createIndexBuffer(idx, ris::BufferUsageFlags::Index);   // after the vertex array exists
        ris::RenderTarget target(8, 8, false);
        target.bind();
        ris::gfx::clear(0, 0, 0, 1, ris::ClearFlags::Color);
        vao.draw(ris::Topology::Triangles, 0, 3);
        REQUIRE(ris_test::read_pixel(4, 4)[0] == 255);
        ris::RenderTarget::unbind();
        GlesUtilities::buffer().deleteBuffer(other);
    }
}

TEST_CASE("GlesBufferUtilities without a window throws std::logic_error") {
    REQUIRE_THROWS_AS(GlesUtilities::buffer().createVertexBuffer(16, ris::BufferUsageFlags::Vertex), std::logic_error);
    REQUIRE_THROWS_AS(GlesUtilities::buffer().createIndexBuffer(16, ris::BufferUsageFlags::Index), std::logic_error);
    REQUIRE_THROWS_AS(GlesUtilities::buffer().createUniformBuffer(16, ris::BufferUsageFlags::Uniform), std::logic_error);
}

TEST_CASE("GlesBufferUtilities details") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;
    auto& buffers = GlesUtilities::buffer();

    SECTION("byte sized index buffer and raw byte uniform buffer") {
        uint32_t index = buffers.createIndexBuffer(12, ris::BufferUsageFlags::Index | ris::BufferUsageFlags::CopyDst, "index");
        REQUIRE(index != 0);
        REQUIRE_NOTHROW(buffers.updateBuffer(ris::BufferType::Index, index, 0, std::vector<uint16_t>{0, 1, 2}.data(), 6));
        buffers.deleteBuffer(index);
        REQUIRE_THROWS_AS(buffers.createIndexBuffer(0, ris::BufferUsageFlags::Index), std::invalid_argument);
        REQUIRE_THROWS_AS(buffers.createIndexBuffer(std::span<const uint16_t>{}, ris::BufferUsageFlags::Index), std::invalid_argument);

        const std::vector<uint8_t> bytes(64, 7);
        uint32_t uniform = buffers.createUniformBuffer(bytes, ris::BufferUsageFlags::Uniform | ris::BufferUsageFlags::CopyDst, "uniform");
        REQUIRE(uniform != 0);
        buffers.deleteBuffer(uniform);
        REQUIRE_THROWS_AS(buffers.createUniformBuffer(std::span<const uint8_t>{}, ris::BufferUsageFlags::Uniform), std::invalid_argument);
        REQUIRE_THROWS_AS(buffers.createUniformBuffer(0, ris::BufferUsageFlags::Uniform), std::invalid_argument);
    }
    SECTION("updateVertexBuffer with source offset and length") {
        const std::vector<float> data = {1, 2, 3, 4, 5, 6};
        uint32_t vb = buffers.createVertexBuffer(24, ris::BufferUsageFlags::Vertex | ris::BufferUsageFlags::CopyDst);
        // dest offset in bytes, source offset and length in floats
        REQUIRE_NOTHROW(buffers.updateVertexBuffer(vb, data, 8, 2, 3));
        REQUIRE_NOTHROW(buffers.updateVertexBuffer(vb, data, 0, 4));        // length 0: the rest of the data
        REQUIRE_NOTHROW(buffers.updateVertexBuffer(vb, data, 0, 0, 6));
        REQUIRE_THROWS_AS(buffers.updateVertexBuffer(vb, data, 0, 4, 3), std::out_of_range);
        REQUIRE_THROWS_AS(buffers.updateVertexBuffer(vb, data, 0, 7), std::out_of_range);
        REQUIRE_THROWS_AS(buffers.updateVertexBuffer(vb, data, 0, 0, 7), std::out_of_range);
        REQUIRE_THROWS_AS(buffers.updateVertexBuffer(vb, data, 0, 6), std::invalid_argument);   // empty range
        // a range that does not fit into the buffer is a GL error
        REQUIRE_THROWS_AS(buffers.updateVertexBuffer(vb, data, 20), std::runtime_error);
        buffers.deleteBuffer(vb);
    }
}
