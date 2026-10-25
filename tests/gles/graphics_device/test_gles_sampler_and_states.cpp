// Tests of GlesSampler, GlesBlendState and GlesPrimitiveState created by the graphics device (need a display).
// Like the tests of the utilities, the GL state is checked by drawing into a render target and reading a pixel.
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#include <ris/ris.hpp>

#include "../gles_test_utils.hpp"

namespace gles = ris::gles;
using ris_test::Pixel;

namespace {

bool near(const Pixel& px, int r, int g, int b, int tolerance = 2) {
    return std::abs(px[0] - r) <= tolerance && std::abs(px[1] - g) <= tolerance && std::abs(px[2] - b) <= tolerance;
}

/// A window and an initialised device.
struct Fixture {
    ris::Window window = ris_test::make_hidden_window();
    gles::GlesGraphicsDevice device;
    Fixture() { device.initialize(); }
};

/// Draws a triangle that covers the viewport and colours it with the texture of unit 0, sampled at (u, 0.5) for
/// every pixel.
class SamplingTriangle {
public:
    SamplingTriangle()
        : vertices_(std::vector<float>{-1, -1, 3, -1, -1, 3}, 8),
          vao_(ris::gles::make_vertex_array_desc(vertices_, layout())),
          shader_(R"(#version 300 es
                layout(location = 0) in vec2 a_position;
                void main() { gl_Position = vec4(a_position, 0.0, 1.0); }
            )",
                  R"(#version 300 es
                precision highp float;
                uniform sampler2D u_texture;
                uniform float u_u;
                out vec4 frag;
                void main() { frag = texture(u_texture, vec2(u_u, 0.5)); }
            )"),
          u_location_(shader_.uniform_location("u_u")) {}

    void draw(float u) {
        shader_.use();
        shader_.set_int(shader_.uniform_location("u_texture"), 0);
        shader_.set_float(u_location_, u);
        vao_.draw(ris::Topology::Triangles, 0, 3);
    }

private:
    static ris::VertexBufferLayout layout() {
        ris::VertexBufferLayout l;
        l.array_stride = 8;
        l.attributes = {{ris::VertexFormat::Float32x2, 0, 0}};
        return l;
    }

    ris::gles::GlesVertexBuffer vertices_;
    ris::VertexArray vao_;
    ris::Shader shader_;
    int32_t u_location_;
};

} // namespace

TEST_CASE("createSampler") {
    if (!ris_test::display_available()) SKIP("no display available");
    Fixture f;

    SECTION("the defaults of ris-framework-ts") {
        const std::unique_ptr<ris::ASampler> sampler(f.device.createSampler({}));
        REQUIRE(sampler != nullptr);
        CHECK(sampler->getMinFilter() == ris::SamplerFilter::Linear);
        CHECK(sampler->getMagFilter() == ris::SamplerFilter::Linear);
        CHECK(sampler->getMipmapFilter() == ris::MipMapSamplerFilter::None);
        CHECK(sampler->getAddressModeU() == ris::SamplerAddressMode::Repeat);
        CHECK(sampler->getAddressModeV() == ris::SamplerAddressMode::Repeat);
        CHECK(sampler->getAddressModeW() == ris::SamplerAddressMode::Repeat);
        CHECK(static_cast<gles::GlesSampler*>(sampler.get())->getGlHandle() != 0);
    }

    SECTION("the getters return the descriptor") {
        ris::SamplerDescriptor descriptor;
        descriptor.minFilter = ris::SamplerFilter::Nearest;
        descriptor.magFilter = ris::SamplerFilter::Linear;
        descriptor.mipMapFilter = ris::MipMapSamplerFilter::Nearest;
        descriptor.addressModeU = ris::SamplerAddressMode::ClampToEdge;
        descriptor.addressModeV = ris::SamplerAddressMode::MirrorRepeat;
        descriptor.addressModeW = ris::SamplerAddressMode::ClampToEdge;
        const std::unique_ptr<ris::ASampler> sampler(f.device.createSampler(descriptor));
        CHECK(sampler->getMinFilter() == ris::SamplerFilter::Nearest);
        CHECK(sampler->getMagFilter() == ris::SamplerFilter::Linear);
        CHECK(sampler->getMipmapFilter() == ris::MipMapSamplerFilter::Nearest);
        CHECK(sampler->getAddressModeU() == ris::SamplerAddressMode::ClampToEdge);
        CHECK(sampler->getAddressModeV() == ris::SamplerAddressMode::MirrorRepeat);
        CHECK(sampler->getAddressModeW() == ris::SamplerAddressMode::ClampToEdge);
    }

    SECTION("ids are unique and increasing") {
        const std::unique_ptr<ris::ASampler> a(f.device.createSampler({}));
        const std::unique_ptr<ris::ASampler> b(f.device.createSampler({}));
        CHECK(a->getId() > 0);
        CHECK(b->getId() == a->getId() + 1);
    }

    SECTION("anisotropy is accepted, without the extension it is ignored") {
        ris::SamplerDescriptor descriptor;
        descriptor.maxAnisotropy = 8.0f;
        const std::unique_ptr<ris::ASampler> sampler(f.device.createSampler(descriptor));
        CHECK(sampler != nullptr);
    }

    SECTION("a maximum anisotropy below 1 or NaN is invalid") {
        ris::SamplerDescriptor descriptor;
        descriptor.maxAnisotropy = 0.5f;
        CHECK_THROWS_AS(f.device.createSampler(descriptor), std::invalid_argument);
        descriptor.maxAnisotropy = -1.0f;
        CHECK_THROWS_AS(f.device.createSampler(descriptor), std::invalid_argument);
        descriptor.maxAnisotropy = std::numeric_limits<float>::quiet_NaN();
        CHECK_THROWS_AS(f.device.createSampler(descriptor), std::invalid_argument);
    }

    SECTION("enum values without OpenGL ES mapping are unsupported") {
        ris::SamplerDescriptor descriptor;
        descriptor.minFilter = static_cast<ris::SamplerFilter>(1234);
        CHECK_THROWS_AS(f.device.createSampler(descriptor), std::domain_error);
        descriptor = {};
        descriptor.addressModeW = static_cast<ris::SamplerAddressMode>(1234);
        CHECK_THROWS_AS(f.device.createSampler(descriptor), std::domain_error);
        descriptor = {};
        descriptor.compareFunction = static_cast<ris::SamplerCompareFunction>(1234);
        CHECK_THROWS_AS(f.device.createSampler(descriptor), std::domain_error);
    }
}

TEST_CASE("a bound sampler decides how the texture is sampled") {
    if (!ris_test::display_available()) SKIP("no display available");
    Fixture f;
    SamplingTriangle triangle;
    // Two texels: red at u = 0.25, blue at u = 0.75.
    const uint8_t pixels[] = {255, 0, 0, 255, 0, 0, 255, 255};
    ris::Texture texture = ris::Texture::from_rgba8(2, 1, pixels, false);
    ris::RenderTarget target(8, 8, false);
    target.bind();
    texture.bind(0);

    const auto sample = [&](const ris::SamplerDescriptor& descriptor, float u) {
        const std::unique_ptr<ris::ASampler> sampler(f.device.createSampler(descriptor));
        static_cast<gles::GlesSampler*>(sampler.get())->bind(0);
        triangle.draw(u);
        return ris_test::read_pixel(4, 4);
    };

    ris::SamplerDescriptor nearest;
    nearest.minFilter = ris::SamplerFilter::Nearest;
    nearest.magFilter = ris::SamplerFilter::Nearest;
    ris::SamplerDescriptor linear; // linear, repeat

    SECTION("nearest picks a texel, linear blends two") {
        CHECK(near(sample(nearest, 0.375f), 255, 0, 0));
        CHECK(near(sample(linear, 0.375f), 191, 0, 64, 3));   // a quarter of the way from red to blue
    }
    SECTION("repeat and clamp differ outside of [0, 1]") {
        ris::SamplerDescriptor repeat = nearest;
        repeat.addressModeU = ris::SamplerAddressMode::Repeat;
        CHECK(near(sample(repeat, 1.25f), 255, 0, 0));    // 0.25
        ris::SamplerDescriptor clamp = nearest;
        clamp.addressModeU = ris::SamplerAddressMode::ClampToEdge;
        CHECK(near(sample(clamp, 1.25f), 0, 0, 255));     // the last texel
        ris::SamplerDescriptor mirror = nearest;
        mirror.addressModeU = ris::SamplerAddressMode::MirrorRepeat;
        CHECK(near(sample(mirror, 1.25f), 0, 0, 255));    // 1.25 mirrors to 0.75
    }
    gles::GlesSampler(linear).bind(0); // makes sure that unit 0 does not keep a deleted sampler
    ris::RenderTarget::unbind();
}

TEST_CASE("primitive state creation and apply") {
    if (!ris_test::display_available()) SKIP("no display available");
    Fixture f;

    SECTION("the descriptor is kept") {
        ris::PrimitiveStateDescriptor descriptor;
        descriptor.topology = ris::PrimitiveTopology::LineStrip;
        descriptor.cullFace = ris::CullMode::Front;
        descriptor.frontFace = ris::FrontFace::Cw;
        const std::unique_ptr<ris::APrimitiveState> state(f.device.createPrimitiveState(descriptor));
        CHECK(state->getTopology() == ris::PrimitiveTopology::LineStrip);
        CHECK(state->getCullFace() == ris::CullMode::Front);
        CHECK(state->getFrontFace() == ris::FrontFace::Cw);
        CHECK(static_cast<gles::GlesPrimitiveState*>(state.get())->getGlPrimitiveType() == 0x0003 /* GL_LINE_STRIP */);
    }

    SECTION("apply sets the culling of the context") {
        ris_test::FullscreenTriangle triangle; // counter clockwise
        ris::RenderTarget target(8, 8, false);
        target.bind();
        const auto drawn = [&](const ris::PrimitiveStateDescriptor& descriptor) {
            const std::unique_ptr<ris::APrimitiveState> state(f.device.createPrimitiveState(descriptor));
            static_cast<gles::GlesPrimitiveState*>(state.get())->apply();
            ris::gfx::clear(0, 0, 0, 1, ris::ClearFlags::Color);
            triangle.draw(1, 0, 0, 1);
            return ris_test::read_pixel(4, 4)[0] == 255;
        };

        ris::PrimitiveStateDescriptor descriptor; // back face culling, counter clockwise front
        CHECK(drawn(descriptor));
        descriptor.cullFace = ris::CullMode::Front;
        CHECK_FALSE(drawn(descriptor));
        descriptor.cullFace = ris::CullMode::None;
        CHECK(drawn(descriptor));
        descriptor.frontFace = ris::FrontFace::Cw; // the triangle is now a back face
        descriptor.cullFace = ris::CullMode::Back;
        CHECK_FALSE(drawn(descriptor));
        descriptor.cullFace = ris::CullMode::Front;
        CHECK(drawn(descriptor));

        // The default primitive state of the device is the OpenGL default plus back face culling.
        static_cast<const gles::GlesPrimitiveState&>(f.device.getDefaultPrimitiveState()).apply();
        ris::gfx::clear(0, 0, 0, 1, ris::ClearFlags::Color);
        triangle.draw(1, 0, 0, 1);
        CHECK(ris_test::read_pixel(4, 4)[0] == 255);
        ris::gfx::set_cull_mode(ris::CullMode::None);
        ris::RenderTarget::unbind();
    }

    SECTION("enum values without OpenGL ES mapping are unsupported") {
        ris::PrimitiveStateDescriptor descriptor;
        descriptor.topology = static_cast<ris::PrimitiveTopology>(99);
        CHECK_THROWS_AS(f.device.createPrimitiveState(descriptor), std::domain_error);
        descriptor = {};
        descriptor.cullFace = static_cast<ris::CullMode>(99);
        CHECK_THROWS_AS(f.device.createPrimitiveState(descriptor), std::domain_error);
        descriptor = {};
        descriptor.frontFace = static_cast<ris::FrontFace>(99);
        CHECK_THROWS_AS(f.device.createPrimitiveState(descriptor), std::domain_error);
    }
}

TEST_CASE("blend state creation and apply") {
    if (!ris_test::display_available()) SKIP("no display available");
    Fixture f;

    SECTION("apply sets blending of the context") {
        ris_test::FullscreenTriangle triangle;
        ris::RenderTarget target(8, 8, false);
        target.bind();
        const auto draw = [&](const ris::BlendStateDescriptor& descriptor, float r, float g, float b, float a) {
            const std::unique_ptr<ris::ABlendState> state(f.device.createBlendState(descriptor));
            CHECK(state->getDescriptor() == descriptor);
            static_cast<gles::GlesBlendState*>(state.get())->apply();
            ris::gfx::clear(0, 0, 1, 1, ris::ClearFlags::Color); // blue background
            triangle.draw(r, g, b, a);
            return ris_test::read_pixel(4, 4);
        };

        // The default is alpha blending.
        CHECK(near(draw({}, 1, 0, 0, 0.5f), 128, 0, 128));

        ris::BlendStateDescriptor off;
        off.blendingEnabled = false;
        CHECK(near(draw(off, 1, 0, 0, 0.5f), 255, 0, 0));

        ris::BlendStateDescriptor additive;
        additive.color = {ris::BlendFactor::One, ris::BlendFactor::One, ris::BlendOperation::Add};
        additive.alpha = additive.color;
        CHECK(near(draw(additive, 1, 0, 0, 1), 255, 0, 255));

        ris::BlendStateDescriptor multiply;
        multiply.color = {ris::BlendFactor::DstColor, ris::BlendFactor::Zero, ris::BlendOperation::Add};
        multiply.alpha = multiply.color;
        CHECK(near(draw(multiply, 1, 1, 0.5f, 1), 0, 0, 128));

        static_cast<const gles::GlesBlendState&>(f.device.getDefaultBlendState()).apply();
        ris::gfx::clear(0, 0, 1, 1, ris::ClearFlags::Color);
        triangle.draw(1, 0, 0, 0.5f);
        CHECK(near(ris_test::read_pixel(4, 4), 128, 0, 128));
        ris::gfx::set_blend_mode(ris::BlendMode::None);
        ris::RenderTarget::unbind();
    }

    SECTION("enum values without OpenGL ES mapping are unsupported") {
        ris::BlendStateDescriptor descriptor;
        descriptor.color.srcFactor = static_cast<ris::BlendFactor>(1234);
        CHECK_THROWS_AS(f.device.createBlendState(descriptor), std::domain_error);
        descriptor = {};
        descriptor.alpha.operation = static_cast<ris::BlendOperation>(1234);
        CHECK_THROWS_AS(f.device.createBlendState(descriptor), std::domain_error);
    }
}
