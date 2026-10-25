// Tests of the TS-style typed buffers (buffers.hpp). Argument validation needs no display, the GL tests
// need a display (or Xvfb) and an EGL implementation like test_window.cpp.
#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <vector>

#include <ris/ris.hpp>

namespace {
bool display_available() {
#if defined(__linux__)
    return std::getenv("DISPLAY") != nullptr || std::getenv("WAYLAND_DISPLAY") != nullptr;
#else
    return true;
#endif
}

ris::Window make_hidden_window() {
    ris::WindowDesc desc;
    desc.width = 64;
    desc.height = 64;
    desc.flags = ris::WindowFlags::Hidden;
    return ris::Window(desc);
}
} // namespace

TEST_CASE("buffer usage flags") {
    using F = ris::BufferUsageFlags;
    const F f = F::Vertex | F::CopyDst;
    REQUIRE(ris::has_flag(f, F::Vertex));
    REQUIRE(ris::has_flag(f, F::CopyDst));
    REQUIRE_FALSE(ris::has_flag(f, F::Index));
    REQUIRE(static_cast<uint32_t>(f) == 33);   // same values as BufferUsage of ris-framework-ts
    REQUIRE(static_cast<uint32_t>(F::Storage) == 8);
}

TEST_CASE("vertex format sizes and layouts") {
    REQUIRE(ris::vertex_format_byte_size(ris::VertexFormat::Float32) == 4);
    REQUIRE(ris::vertex_format_byte_size(ris::VertexFormat::Float32x3) == 12);
    REQUIRE(ris::vertex_format_byte_size(ris::VertexFormat::Float32x16) == 64);

    const auto l = ris::VertexBufferLayout::create_float3_float4_float2_float3();
    REQUIRE(l.array_stride == 48);
    REQUIRE(l.attributes.size() == 4);
    REQUIRE(l.attributes[3].offset == 36);
    REQUIRE(ris::VertexBufferLayout::create_float3_float2().array_stride == 20);
    REQUIRE(ris::VertexBufferLayout::create_float3_float4_float2().array_stride == 36);

    const auto attribs = l.to_vertex_attribs(2);
    REQUIRE(attribs.size() == 4);
    REQUIRE(attribs[1].location == 1);
    REQUIRE(attribs[1].components == 4);
    REQUIRE(attribs[1].offset == 12);
    REQUIRE(attribs[1].buffer_slot == 2);
    REQUIRE(attribs[1].divisor == 0);

    SECTION("a mat4 expands to four vec4 columns, instance step sets the divisor") {
        ris::VertexBufferLayout m;
        m.array_stride = 64;
        m.step_mode = ris::VertexStepMode::Instance;
        m.attributes = {{ris::VertexFormat::Float32x16, 0, 4}};
        const auto a = m.to_vertex_attribs();
        REQUIRE(a.size() == 4);
        for (uint32_t i = 0; i < 4; ++i) {
            REQUIRE(a[i].location == 4 + i);
            REQUIRE(a[i].offset == 16 * i);
            REQUIRE(a[i].components == 4);
            REQUIRE(a[i].divisor == 1);
        }
    }
    SECTION("attributes must fit the stride") {
        ris::VertexBufferLayout bad;
        bad.array_stride = 16;
        bad.attributes = {{ris::VertexFormat::Float32x4, 4, 0}};
        REQUIRE_THROWS_AS(bad.to_vertex_attribs(), ris::Error);
    }
}

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

TEST_CASE("typed buffers on a real context") {
    if (!display_available()) SKIP("no display available");
    ris::Window window = make_hidden_window();
    using F = ris::BufferUsageFlags;

    SECTION("vertex buffer") {
        const std::vector<float> data(5 * 4, 0.5f);   // 4 vertices of float3 + float2
        ris::VertexBuffer vb(data, 20, F::Vertex | F::CopyDst, "quad");
        REQUIRE(vb.label() == "quad");
        REQUIRE(vb.vertex_count() == 4);
        REQUIRE(vb.byte_stride() == 20);
        REQUIRE(vb.byte_size() == 80);
        REQUIRE(vb.gl_handle() != 0);
        REQUIRE(ris::has_flag(vb.usage(), F::Vertex));
        REQUIRE(ris::has_flag(vb.usage(), F::CopyDst));

        const std::vector<float> one(5, 1.0f);
        REQUIRE_NOTHROW(vb.update(one, 60));                             // last vertex
        REQUIRE_THROWS_AS(vb.update(one, 64), ris::Error);               // 4 bytes too far
        REQUIRE_THROWS_AS(vb.update(std::span<const float>{}, 0), ris::Error);   // null data

        ris::VertexBuffer moved(std::move(vb));
        REQUIRE(moved.vertex_count() == 4);
        REQUIRE(vb.gl_handle() == 0);   // NOLINT moved-from

        auto dynamic = ris::VertexBuffer::with_byte_size(40, 20);
        REQUIRE(dynamic.vertex_count() == 2);
        REQUIRE_NOTHROW(dynamic.update(one));
    }
    SECTION("vertex buffer without CopyDst cannot be updated") {
        const std::vector<float> data(5, 0.0f);
        ris::VertexBuffer vb(data, 20);
        REQUIRE_THROWS_AS(vb.update(data), ris::Error);
    }
    SECTION("index buffer picks the smallest type") {
        const std::vector<uint32_t> small = {0, 1, 2, 2, 1, 65535};
        ris::IndexBuffer a(small);
        REQUIRE(a.type() == ris::IndexBufferType::UInt16);
        REQUIRE(a.element_byte_size() == 2);
        REQUIRE(a.indices_count() == 6);
        REQUIRE(a.byte_size() == 12);
        REQUIRE(a.index_type() == ris::IndexType::UInt16);
        REQUIRE(ris::has_flag(a.usage(), F::Index));

        const std::vector<uint32_t> big = {0, 1, 65536};
        ris::IndexBuffer b(big, "big");
        REQUIRE(b.type() == ris::IndexBufferType::UInt32);
        REQUIRE(b.element_byte_size() == 4);
        REQUIRE(b.indices_count() == 3);
        REQUIRE(b.label() == "big");

        auto forced = ris::IndexBuffer::create_uint32(small);
        REQUIRE(forced.type() == ris::IndexBufferType::UInt32);
        REQUIRE(forced.byte_size() == 24);

        const std::vector<uint16_t> u16 = {0, 1, 2};
        REQUIRE(ris::IndexBuffer(u16).type() == ris::IndexBufferType::UInt16);
    }
    SECTION("uniform buffer is padded to 16 bytes") {
        const std::vector<float> data(5, 1.0f);   // 20 bytes
        ris::UniformBuffer ub(data);
        REQUIRE(ub.byte_size() == 32);
        REQUIRE(ris::has_flag(ub.usage(), F::Uniform));
        REQUIRE_NOTHROW(ub.update(ris::Vec4(1, 2, 3, 4), 16));
        REQUIRE_NOTHROW(ub.update(data, 12));
        REQUIRE_THROWS_AS(ub.update(ris::Vec4(1), 20), ris::Error);

        auto sized = ris::UniformBuffer::with_byte_size(65);
        REQUIRE(sized.byte_size() == 80);
        REQUIRE_NOTHROW(sized.update(ris::Mat4(1.0f), 16));
        REQUIRE_THROWS_AS(sized.update(ris::Mat4(1.0f), 32), ris::Error);

        ris::UniformBuffer fixed(data, F::Uniform);   // no CopyDst
        REQUIRE_THROWS_AS(fixed.update(data), ris::Error);
    }
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
    SECTION("vertex array description checks the stride") {
        const std::vector<float> data(10, 0.0f);
        ris::VertexBuffer vb(data, 20);
        REQUIRE_THROWS_AS(ris::make_vertex_array_desc(vb, ris::VertexBufferLayout::create_float3_float4_float2()),
                          ris::Error);
    }
}

TEST_CASE("typed buffers draw a quad with a uniform block") {
    if (!display_available()) SKIP("no display available");
    ris::Window window = make_hidden_window();

    // two triangles covering the whole viewport: position (vec3) + uv (vec2)
    const std::vector<float> vertices = {
        -1, -1, 0, 0, 0,
         1, -1, 0, 1, 0,
         1,  1, 0, 1, 1,
        -1,  1, 0, 0, 1,
    };
    const std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};
    const auto layout = ris::VertexBufferLayout::create_float3_float2();

    ris::VertexBuffer vb(vertices, layout.array_stride);
    ris::IndexBuffer ib(indices);
    ris::UniformBuffer color(std::span<const float>(std::vector<float>{1, 0, 0, 1}));
    ris::VertexArray vao(ris::make_vertex_array_desc(vb, layout, &ib));

    ris::Shader shader(R"(#version 300 es
        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec2 a_uv;
        void main() { gl_Position = vec4(a_position, 1.0); }
    )",
                       R"(#version 300 es
        precision mediump float;
        layout(std140) uniform Color { vec4 color; };
        out vec4 frag;
        void main() { frag = color; }
    )");
    shader.use();
    shader.bind_uniform_block("Color", 0);
    color.bind_base(0);

    ris::RenderTarget target(16, 16, false);
    target.bind();

    const auto draw_and_read = [&] {
        ris::gfx::clear(0, 0, 0, 1, ris::ClearFlags::Color);
        vao.draw(ris::Topology::Triangles, 0, ib.indices_count());
        return ris::gfx::read_pixels(8, 8, 1, 1);
    };

    auto px = draw_and_read();
    REQUIRE(px[0] == 255);
    REQUIRE(px[1] == 0);
    REQUIRE(px[2] == 0);

    color.update(ris::Vec4(0, 1, 0, 1));
    px = draw_and_read();
    REQUIRE(px[0] == 0);
    REQUIRE(px[1] == 255);

    ris::RenderTarget::unbind();
}
