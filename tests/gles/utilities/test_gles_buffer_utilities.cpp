// Tests of GlesBufferUtilities (need a display).
#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <vector>

#include <ris/gles/gles.hpp>

#include "../gles_test_utils.hpp"

namespace gles = ris::gles;
using F = ris::BufferUsageFlags;

TEST_CASE("GlesBufferUtilities create, update and delete buffers") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();

    SECTION("vertex buffers") {
        const std::vector<float> data(12, 1.0f);
        gles::GlUint vb = GlesUtilities::buffer().createVertexBuffer(data, F::Vertex | F::CopyDst, "vb");
        REQUIRE(vb != 0);
        REQUIRE_NOTHROW(GlesUtilities::buffer().updateVertexBuffer(vb, std::vector<float>{2, 3}, 40));   // last two floats
        REQUIRE_NOTHROW(GlesUtilities::buffer().updateVertexBuffer(vb, std::vector<float>{2, 3}));
        REQUIRE_THROWS_AS(GlesUtilities::buffer().updateVertexBuffer(vb, std::span<const float>{}), std::invalid_argument);
        REQUIRE_THROWS_AS(GlesUtilities::buffer().updateVertexBuffer(0, data), std::invalid_argument);
        GlesUtilities::buffer().deleteBuffer(vb);
        REQUIRE(vb == 0);
        REQUIRE_NOTHROW(GlesUtilities::buffer().deleteBuffer(vb));   // deleting 0 does nothing

        gles::GlUint empty = GlesUtilities::buffer().createVertexBuffer(64, F::Vertex);
        REQUIRE(empty != 0);
        GlesUtilities::buffer().deleteBuffer(empty);
        REQUIRE_THROWS_AS(GlesUtilities::buffer().createVertexBuffer(std::span<const float>{}, F::Vertex), std::invalid_argument);
        REQUIRE_THROWS_AS(GlesUtilities::buffer().createVertexBuffer(0, F::Vertex), std::invalid_argument);
    }
    SECTION("index buffers") {
        const std::vector<uint16_t> narrow = {0, 1, 2};
        const std::vector<uint32_t> wide = {0, 1, 2, 3};
        gles::GlUint a = GlesUtilities::buffer().createIndexBuffer(narrow, F::Index);
        gles::GlUint b = GlesUtilities::buffer().createIndexBuffer(wide, F::Index | F::CopyDst);
        REQUIRE(a != 0);
        REQUIRE(b != 0);
        REQUIRE(a != b);
        REQUIRE_NOTHROW(GlesUtilities::buffer().updateBuffer(ris::BufferType::Index, b, 4, wide.data(), 8));
        GlesUtilities::buffer().deleteBuffer(a);
        GlesUtilities::buffer().deleteBuffer(b);
    }
    SECTION("uniform buffers") {
        const std::vector<float> data = {1, 2, 3, 4};
        gles::GlUint ub = GlesUtilities::buffer().createUniformBuffer(data, F::Uniform | F::CopyDst, "ub");
        REQUIRE(ub != 0);
        REQUIRE_NOTHROW(GlesUtilities::buffer().updateBuffer(ris::BufferType::Uniform, ub, 0, data.data(), 16));
        GlesUtilities::buffer().deleteBuffer(ub);
        gles::GlUint sized = GlesUtilities::buffer().createUniformBuffer(32, F::Uniform);
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
        gles::GlUint other = GlesUtilities::buffer().createIndexBuffer(idx, F::Index);   // after the vertex array exists
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
    REQUIRE_THROWS_AS(GlesUtilities::buffer().createVertexBuffer(16, F::Vertex), std::logic_error);
    REQUIRE_THROWS_AS(GlesUtilities::buffer().createIndexBuffer(16, F::Index), std::logic_error);
    REQUIRE_THROWS_AS(GlesUtilities::buffer().createUniformBuffer(16, F::Uniform), std::logic_error);
}

TEST_CASE("GlesBufferUtilities details") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    auto& buffers = GlesUtilities::buffer();

    SECTION("byte sized index buffer and raw byte uniform buffer") {
        gles::GlUint index = buffers.createIndexBuffer(12, F::Index | F::CopyDst, "index");
        REQUIRE(index != 0);
        REQUIRE_NOTHROW(buffers.updateBuffer(ris::BufferType::Index, index, 0, std::vector<uint16_t>{0, 1, 2}.data(), 6));
        buffers.deleteBuffer(index);
        REQUIRE_THROWS_AS(buffers.createIndexBuffer(0, F::Index), std::invalid_argument);
        REQUIRE_THROWS_AS(buffers.createIndexBuffer(std::span<const uint16_t>{}, F::Index), std::invalid_argument);

        const std::vector<uint8_t> bytes(64, 7);
        gles::GlUint uniform = buffers.createUniformBuffer(bytes, F::Uniform | F::CopyDst, "uniform");
        REQUIRE(uniform != 0);
        buffers.deleteBuffer(uniform);
        REQUIRE_THROWS_AS(buffers.createUniformBuffer(std::span<const uint8_t>{}, F::Uniform), std::invalid_argument);
        REQUIRE_THROWS_AS(buffers.createUniformBuffer(0, F::Uniform), std::invalid_argument);
    }
    SECTION("updateVertexBuffer with source offset and length") {
        const std::vector<float> data = {1, 2, 3, 4, 5, 6};
        gles::GlUint vb = buffers.createVertexBuffer(24, F::Vertex | F::CopyDst);
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
