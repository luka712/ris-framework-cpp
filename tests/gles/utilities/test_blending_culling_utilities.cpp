// Tests of blending_utilities and culling_utilities (need a display). The state is checked by drawing a
// triangle into a render target and reading a pixel back.
#include <catch2/catch_test_macros.hpp>

#include <cstdlib>

#include <ris/gles/gles.hpp>

#include "../gles_test_utils.hpp"

namespace gles = ris::gles;
namespace bl = ris::gles::blending_utilities;
namespace cu = ris::gles::culling_utilities;

namespace {

bool near(const std::array<int, 4>& px, int r, int g, int b, int tolerance = 2) {
    return std::abs(px[0] - r) <= tolerance && std::abs(px[1] - g) <= tolerance && std::abs(px[2] - b) <= tolerance;
}

} // namespace

TEST_CASE("blending utilities") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    ris_test::FullscreenTriangle triangle;
    ris::RenderTarget target(8, 8, false);
    target.bind();
    const auto draw = [&](float r, float g, float b, float a) {
        ris::gfx::clear(0, 0, 1, 1, ris::ClearFlags::Color);   // blue background
        triangle.draw(r, g, b, a);
        return ris_test::read_pixel(4, 4);
    };

    SECTION("the defaults are alpha blending") {
        bl::enable_blend();
        REQUIRE(near(draw(1, 0, 0, 0.5f), 128, 0, 128));
        bl::disable_blend();
    }
    SECTION("disabled blending overwrites") {
        bl::enable_blend();
        bl::disable_blend();
        REQUIRE(near(draw(1, 0, 0, 0.5f), 255, 0, 0));
    }
    SECTION("custom factors and operations") {
        bl::BlendParams additive;
        additive.color = {ris::BlendOperation::Add, ris::BlendFactor::One, ris::BlendFactor::One};
        additive.alpha = additive.color;
        bl::enable_blend(additive);
        REQUIRE(near(draw(1, 0, 0, 1), 255, 0, 255));

        bl::BlendParams subtract;
        subtract.color = {ris::BlendOperation::ReverseSubtract, ris::BlendFactor::One, ris::BlendFactor::One};   // dst - src
        subtract.alpha = subtract.color;
        bl::enable_blend(subtract);
        REQUIRE(near(draw(0, 0, 1, 1), 0, 0, 0));

        bl::BlendParams multiply;
        multiply.color = {ris::BlendOperation::Add, ris::BlendFactor::DstColor, ris::BlendFactor::Zero};
        multiply.alpha = multiply.color;
        bl::enable_blend(multiply);
        REQUIRE(near(draw(1, 1, 0.5f, 1), 0, 0, 128));   // blue * (1, 1, 0.5)
        bl::disable_blend();
    }
    SECTION("gfx::set_blend_mode goes through the same utilities") {
        ris::gfx::set_blend_mode(ris::BlendMode::Additive);   // SRC_ALPHA, ONE
        REQUIRE(near(draw(1, 0, 0, 1), 255, 0, 255));
        ris::gfx::set_blend_mode(ris::BlendMode::Multiply);
        REQUIRE(near(draw(1, 1, 0.5f, 1), 0, 0, 128));
        ris::gfx::set_blend_mode(ris::BlendMode::Premultiplied);   // ONE, ONE_MINUS_SRC_ALPHA
        REQUIRE(near(draw(0.5f, 0, 0, 0.5f), 128, 0, 128));
        ris::gfx::set_blend_mode(ris::BlendMode::None);
        REQUIRE(near(draw(1, 0, 0, 0.5f), 255, 0, 0));
    }
    ris::RenderTarget::unbind();
}

TEST_CASE("culling utilities") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    ris_test::FullscreenTriangle triangle;   // counter clockwise
    ris::RenderTarget target(8, 8, false);
    target.bind();
    const auto drawn = [&] {
        ris::gfx::clear(0, 0, 0, 1, ris::ClearFlags::Color);
        triangle.draw(1, 0, 0, 1);
        return ris_test::read_pixel(4, 4)[0] == 255;
    };

    cu::set_front_face(ris::FrontFace::Ccw);
    SECTION("counter clockwise front faces") {
        cu::set_culling(ris::CullMode::None);
        REQUIRE(drawn());
        cu::set_culling(ris::CullMode::Back);
        REQUIRE(drawn());
        cu::set_culling(ris::CullMode::Front);
        REQUIRE_FALSE(drawn());
    }
    SECTION("clockwise front faces") {
        cu::set_front_face(ris::FrontFace::Cw);   // the triangle is now a back face
        cu::set_culling(ris::CullMode::Back);
        REQUIRE_FALSE(drawn());
        cu::set_culling(ris::CullMode::Front);
        REQUIRE(drawn());
        cu::set_culling(ris::CullMode::None);
        REQUIRE(drawn());
    }
    SECTION("gfx::set_cull_mode") {
        ris::gfx::set_cull_mode(ris::CullMode::Back);
        REQUIRE(drawn());
        ris::gfx::set_cull_mode(ris::CullMode::Front);
        REQUIRE_FALSE(drawn());
        ris::gfx::set_cull_mode(ris::CullMode::None);
        REQUIRE(drawn());
    }
    cu::set_culling(ris::CullMode::None);
    cu::set_front_face(ris::FrontFace::Ccw);
    ris::RenderTarget::unbind();
}
