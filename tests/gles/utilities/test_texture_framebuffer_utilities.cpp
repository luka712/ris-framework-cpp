// Tests of renderbuffer_utilities, framebuffer_utilities and sampler_utilities (need a display). The textures
// come from GlesTextureUtilities (tests/gles/texture/test_gles_texture_utilities.cpp tests that one).
#include <catch2/catch_test_macros.hpp>

#include <vector>

#include <ris/gles/gles.hpp>

#include "../gles_test_utils.hpp"

namespace gles = ris::gles;
namespace fu = ris::gles::framebuffer_utilities;
using ris::TextureFormat;

TEST_CASE("renderbuffer and framebuffer utilities") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();

    SECTION("renderbuffers") {
        uint32_t rb = gles::renderbuffer_utilities::create(TextureFormat::Depth24Stencil8, 16, 16, "depth");
        REQUIRE(rb != 0);
        gles::renderbuffer_utilities::delete_renderbuffer(rb);
        REQUIRE(rb == 0);
        REQUIRE_THROWS_AS(gles::renderbuffer_utilities::create(TextureFormat::Depth24Stencil8, 0, 16), std::invalid_argument);
        REQUIRE_THROWS_AS(gles::renderbuffer_utilities::create(TextureFormat::Undefined, 16, 16), std::domain_error);
    }
    SECTION("a colour framebuffer renders to its texture") {
        uint32_t tex = GlesUtilities::texture().createTexture2D(8, 8, static_cast<const void*>(nullptr));
        uint32_t fb = fu::create(tex, "target");
        REQUIRE(fb != 0);
        fu::bind(fb);
        ris::gfx::clear(0.0f, 1.0f, 0.0f, 1.0f, ris::ClearFlags::Color);
        REQUIRE(ris_test::read_pixel(3, 3) == std::array<int, 4>{0, 255, 0, 255});
        fu::bind(0);
        fu::delete_framebuffer(fb);
        GlesUtilities::texture().deleteTexture(tex);
    }
    SECTION("the previous framebuffer binding is restored") {
        ris::RenderTarget target(8, 8, false);
        target.bind();
        uint32_t tex = GlesUtilities::texture().createTexture2D(8, 8, static_cast<const void*>(nullptr));
        uint32_t fb = fu::create(tex);
        ris::gfx::clear(1.0f, 0.0f, 0.0f, 1.0f, ris::ClearFlags::Color);   // still the render target
        REQUIRE(ris_test::read_pixel(0, 0) == std::array<int, 4>{255, 0, 0, 255});
        fu::delete_framebuffer(fb);
        GlesUtilities::texture().deleteTexture(tex);
        ris::RenderTarget::unbind();
    }
    SECTION("multiple colour attachments") {
        uint32_t a = GlesUtilities::texture().createTexture2D(4, 4, static_cast<const void*>(nullptr));
        uint32_t b = GlesUtilities::texture().createTexture2D(4, 4, static_cast<const void*>(nullptr));
        const std::vector<uint32_t> textures = {a, b};
        uint32_t fb = fu::create(textures);
        REQUIRE(fb != 0);
        fu::delete_framebuffer(fb);
        GlesUtilities::texture().deleteTexture(a);
        GlesUtilities::texture().deleteTexture(b);
    }
    SECTION("depth buffers") {
        uint32_t color = GlesUtilities::texture().createTexture2D(8, 8, static_cast<const void*>(nullptr));
        uint32_t rb = gles::renderbuffer_utilities::create(TextureFormat::Depth24Stencil8, 8, 8);
        uint32_t with_rb = fu::create_with_renderbuffer_as_depth_buffer(color, gles::DepthAttachmentType::DepthStencil, rb);
        REQUIRE(with_rb != 0);

        uint32_t depth_tex = GlesUtilities::texture().createTexture2D(8, 8, static_cast<const void*>(nullptr), TextureFormat::Depth32Float);
        uint32_t with_tex = fu::create_with_texture_as_depth_buffer(color, gles::DepthAttachmentType::Depth, depth_tex);
        REQUIRE(with_tex != 0);

        // the attach functions on a colour-only framebuffer
        uint32_t plain = fu::create(color);
        REQUIRE_NOTHROW(fu::attach_depth_stencil_texture(plain, depth_tex, TextureFormat::Depth32Float));
        uint32_t plain2 = fu::create(color);
        REQUIRE_NOTHROW(fu::attach_depth_stencil_renderbuffer(plain2, rb, TextureFormat::Depth24Stencil8));
        REQUIRE_THROWS_AS(fu::attach_depth_stencil_renderbuffer(plain2, rb, TextureFormat::Rgba8Unorm), std::domain_error);
        REQUIRE_THROWS_AS(fu::attach_depth_stencil_texture(plain2, depth_tex, TextureFormat::Astc4x4Rgba), std::domain_error);

        REQUIRE(fu::depth_attachment_type_for(TextureFormat::Depth32Float) == gles::DepthAttachmentType::Depth);
        REQUIRE(fu::depth_attachment_type_for(TextureFormat::Depth24Stencil8) == gles::DepthAttachmentType::DepthStencil);
        REQUIRE_THROWS_AS(fu::depth_attachment_type_for(TextureFormat::Rgba8Unorm), std::domain_error);

        for (uint32_t* fb : {&with_rb, &with_tex, &plain, &plain2}) fu::delete_framebuffer(*fb);
        gles::renderbuffer_utilities::delete_renderbuffer(rb);
        GlesUtilities::texture().deleteTexture(depth_tex);
        GlesUtilities::texture().deleteTexture(color);
    }
    SECTION("invalid arguments and incomplete framebuffers") {
        REQUIRE_THROWS_AS(fu::create(std::span<const uint32_t>{}), std::invalid_argument);
        REQUIRE_THROWS_AS(fu::create(uint32_t{0}), std::invalid_argument);
        // a colour texture attached as depth is incomplete
        uint32_t color = GlesUtilities::texture().createTexture2D(8, 8, static_cast<const void*>(nullptr));
        REQUIRE_THROWS_AS(fu::create_with_texture_as_depth_buffer(color, gles::DepthAttachmentType::Depth, color), std::runtime_error);
        REQUIRE_NOTHROW(gles::error_utilities::check_error("no stray error"));
        GlesUtilities::texture().deleteTexture(color);
    }
}

TEST_CASE("render target on the gles utilities") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();

    ris::RenderTarget target(16, 16, true);   // with a depth stencil render buffer
    target.bind();
    ris::gfx::clear(0.0f, 0.0f, 1.0f, 1.0f);
    REQUIRE(ris_test::read_pixel(8, 8) == std::array<int, 4>{0, 0, 255, 255});
    ris::RenderTarget::unbind();
}

TEST_CASE("sampler utilities") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    namespace su = ris::gles::sampler_utilities;

    SECTION("defaults") {
        uint32_t sampler = su::create();
        REQUIRE(sampler != 0);
        su::delete_sampler(sampler);
        REQUIRE(sampler == 0);
        REQUIRE_NOTHROW(su::delete_sampler(sampler));
    }
    SECTION("all parameters") {
        su::SamplerParams params;
        params.min_filter = ris::SamplerFilter::Nearest;
        params.mag_filter = ris::SamplerFilter::Nearest;
        params.mip_map_filter = ris::MipMapSamplerFilter::Linear;
        params.address_mode_u = ris::SamplerAddressMode::Repeat;
        params.address_mode_v = ris::SamplerAddressMode::MirrorRepeat;
        params.address_mode_w = ris::SamplerAddressMode::ClampToBorder;
        params.compare_function = ris::SamplerCompareFunction::LessEqual;
        params.anisotropy = 16.0f;
        uint32_t sampler = su::create(params, "shadow sampler");
        REQUIRE(sampler != 0);
        REQUIRE_NOTHROW(gles::error_utilities::check_error("sampler"));
        su::delete_sampler(sampler);
    }
}
