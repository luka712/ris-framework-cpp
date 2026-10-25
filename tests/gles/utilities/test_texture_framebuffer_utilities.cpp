// Tests of texture_utilities, renderbuffer_utilities, framebuffer_utilities and sampler_utilities
// (need a display). Textures are read back through a framebuffer.
#include <catch2/catch_test_macros.hpp>

#include <vector>

#include <ris/gles/gles.hpp>

#include "../gles_test_utils.hpp"

namespace gles = ris::gles;
namespace tu = ris::gles::texture_utilities;
namespace fu = ris::gles::framebuffer_utilities;
using ris::TextureFormat;

namespace {

std::vector<uint8_t> solid(int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    std::vector<uint8_t> px;
    for (int i = 0; i < w * h; ++i) px.insert(px.end(), {r, g, b, a});
    return px;
}

/// Reads the pixel (x, y) of the texture through a temporary framebuffer.
ris_test::Pixel texel(gles::GlUint texture, int x, int y) {
    gles::GlUint fb = fu::create(texture);
    fu::bind(fb);
    const ris_test::Pixel px = ris_test::read_pixel(x, y);
    fu::bind(0);
    fu::delete_framebuffer(fb);
    return px;
}

} // namespace

TEST_CASE("texture utilities create 2d textures") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();

    SECTION("mip count") {
        REQUIRE(tu::full_mip_count(1, 1) == 1);
        REQUIRE(tu::full_mip_count(2, 1) == 2);
        REQUIRE(tu::full_mip_count(256, 256) == 9);
        REQUIRE(tu::full_mip_count(300, 20) == 9);
    }
    SECTION("pixels are uploaded") {
        const auto red = solid(4, 4, 255, 0, 0, 255);
        gles::GlUint tex = tu::create_texture_2d(4, 4, red.data(), TextureFormat::Rgba8Unorm, false, 1.0f, "red");
        REQUIRE(tex != 0);
        REQUIRE(texel(tex, 2, 2) == ris_test::Pixel{255, 0, 0, 255});
        tu::delete_texture(tex);
        REQUIRE(tex == 0);
    }
    SECTION("mip levels are uploaded one by one") {
        const auto level0 = solid(4, 4, 255, 0, 0, 255);
        const auto level1 = solid(2, 2, 0, 255, 0, 255);
        const auto level2 = solid(1, 1, 0, 0, 255, 255);
        const std::vector<tu::MipLevelData> levels = {level0, level1, level2};
        gles::GlUint tex = tu::create_texture_2d(4, 4, levels);
        REQUIRE(tex != 0);
        REQUIRE_NOTHROW(gles::error_utilities::check_error("mips"));
        tu::delete_texture(tex);
    }
    SECTION("an empty level is left uninitialised") {
        const auto level0 = solid(2, 2, 1, 2, 3, 4);
        const std::vector<tu::MipLevelData> levels = {level0, tu::MipLevelData{}};
        gles::GlUint tex = tu::create_texture_2d(2, 2, levels);
        REQUIRE(texel(tex, 0, 0) == ris_test::Pixel{1, 2, 3, 4});
        tu::delete_texture(tex);
    }
    SECTION("generated mip maps and uninitialised textures") {
        const auto px = solid(8, 8, 9, 9, 9, 255);
        gles::GlUint mips = tu::create_texture_2d(8, 8, px.data(), TextureFormat::Rgba8UnormSrgb, true);
        REQUIRE(mips != 0);
        tu::delete_texture(mips);
        gles::GlUint blank = tu::create_texture_2d(8, 8, static_cast<const void*>(nullptr));
        REQUIRE(blank != 0);
        tu::delete_texture(blank);
    }
    SECTION("depth textures") {
        gles::GlUint depth = tu::create_texture_2d(8, 8, static_cast<const void*>(nullptr), TextureFormat::Depth32Float);
        REQUIRE(depth != 0);
        tu::delete_texture(depth);
        REQUIRE_THROWS_AS(
            tu::create_texture_2d(8, 8, static_cast<const void*>(nullptr), TextureFormat::Depth32Float, true), std::invalid_argument);
    }
    SECTION("anisotropy is optional") {
        gles::GlUint tex = tu::create_texture_2d(4, 4, static_cast<const void*>(nullptr), TextureFormat::Rgba8Unorm, false, 8.0f);
        REQUIRE(tex != 0);
        tu::delete_texture(tex);
        const float max = gles::anisotropic_filter_utilities::get_max_anisotropy();
        REQUIRE(max >= 0.0f);
        REQUIRE(gles::anisotropic_filter_utilities::is_supported() == (max > 0.0f));
    }
    SECTION("invalid arguments") {
        const auto small = solid(1, 1, 0, 0, 0, 0);
        REQUIRE_THROWS_AS(tu::create_texture_2d(0, 4, static_cast<const void*>(nullptr)), std::invalid_argument);
        REQUIRE_THROWS_AS(tu::create_texture_2d(4, -1, static_cast<const void*>(nullptr)), std::invalid_argument);
        REQUIRE_THROWS_AS(tu::create_texture_2d(4, 4, static_cast<const void*>(nullptr), TextureFormat::Astc4x4Rgba), std::domain_error);
        REQUIRE_THROWS_AS(tu::create_texture_2d(4, 4, static_cast<const void*>(nullptr), TextureFormat::Red32Float), std::domain_error);
        const std::vector<tu::MipLevelData> too_small = {small};
        REQUIRE_THROWS_AS(tu::create_texture_2d(4, 4, too_small), std::invalid_argument);
        const std::vector<tu::MipLevelData> too_many = {small, small, small, small};
        REQUIRE_THROWS_AS(tu::create_texture_2d(4, 4, too_many), std::invalid_argument);   // only 3 levels fit
        REQUIRE_NOTHROW(gles::error_utilities::check_error("no stray error"));
    }
}

TEST_CASE("texture utilities create compressed textures") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();

    // ETC2 RGBA8 is core in OpenGL ES 3 (and emulated by ANGLE): 16 bytes per 4x4 block.
    const std::vector<uint8_t> block(16, 0);
    SECTION("a multiple of the block size uses immutable storage") {
        const std::vector<uint8_t> level0(16 * 4, 0);   // 8x8 pixels = 4 blocks
        const std::vector<tu::MipLevelData> levels = {level0, block, block, block};
        gles::GlUint tex = tu::create_compressed_texture_2d(8, 8, 4, 4, levels, TextureFormat::Etc2Rgba8Unorm);
        REQUIRE(tex != 0);
        tu::delete_texture(tex);
    }
    SECTION("other sizes are uploaded rounded up to whole blocks") {
        const std::vector<tu::MipLevelData> levels = {block};   // 3x3 -> one block
        gles::GlUint tex = tu::create_compressed_texture_2d(3, 3, 4, 4, levels, TextureFormat::Etc2Rgba8Unorm);
        REQUIRE(tex != 0);
        tu::delete_texture(tex);
    }
    SECTION("invalid arguments") {
        const std::vector<tu::MipLevelData> levels = {block};
        REQUIRE_THROWS_AS(tu::create_compressed_texture_2d(4, 4, 4, 4, {}, TextureFormat::Etc2Rgba8Unorm), std::invalid_argument);
        REQUIRE_THROWS_AS(tu::create_compressed_texture_2d(4, 4, 4, 4, levels, TextureFormat::Rgba8Unorm), std::domain_error);
        REQUIRE_THROWS_AS(tu::create_compressed_texture_2d(0, 4, 4, 4, levels, TextureFormat::Etc2Rgba8Unorm), std::invalid_argument);
        REQUIRE_THROWS_AS(tu::create_compressed_texture_2d(4, 4, 0, 4, levels, TextureFormat::Etc2Rgba8Unorm), std::invalid_argument);
        const std::vector<tu::MipLevelData> empty_level = {tu::MipLevelData{}};
        REQUIRE_THROWS_AS(tu::create_compressed_texture_2d(4, 4, 4, 4, empty_level, TextureFormat::Etc2Rgba8Unorm), std::invalid_argument);
        // a block that is too short for the size makes GL fail: reported as Error, nothing leaks, no stray error
        const std::vector<uint8_t> short_block(4, 0);
        const std::vector<tu::MipLevelData> short_levels = {short_block};
        REQUIRE_THROWS_AS(tu::create_compressed_texture_2d(4, 4, 4, 4, short_levels, TextureFormat::Etc2Rgba8Unorm), std::runtime_error);
        REQUIRE_NOTHROW(gles::error_utilities::check_error("no stray error"));
    }
}

TEST_CASE("renderbuffer and framebuffer utilities") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();

    SECTION("renderbuffers") {
        gles::GlUint rb = gles::renderbuffer_utilities::create(TextureFormat::Depth24Stencil8, 16, 16, "depth");
        REQUIRE(rb != 0);
        gles::renderbuffer_utilities::delete_renderbuffer(rb);
        REQUIRE(rb == 0);
        REQUIRE_THROWS_AS(gles::renderbuffer_utilities::create(TextureFormat::Depth24Stencil8, 0, 16), std::invalid_argument);
        REQUIRE_THROWS_AS(gles::renderbuffer_utilities::create(TextureFormat::Undefined, 16, 16), std::domain_error);
    }
    SECTION("a colour framebuffer renders to its texture") {
        gles::GlUint tex = tu::create_texture_2d(8, 8, static_cast<const void*>(nullptr));
        gles::GlUint fb = fu::create(tex, "target");
        REQUIRE(fb != 0);
        fu::bind(fb);
        ris::gfx::clear(0.0f, 1.0f, 0.0f, 1.0f, ris::ClearFlags::Color);
        REQUIRE(ris_test::read_pixel(3, 3) == ris_test::Pixel{0, 255, 0, 255});
        fu::bind(0);
        fu::delete_framebuffer(fb);
        tu::delete_texture(tex);
    }
    SECTION("the previous framebuffer binding is restored") {
        ris::RenderTarget target(8, 8, false);
        target.bind();
        gles::GlUint tex = tu::create_texture_2d(8, 8, static_cast<const void*>(nullptr));
        gles::GlUint fb = fu::create(tex);
        ris::gfx::clear(1.0f, 0.0f, 0.0f, 1.0f, ris::ClearFlags::Color);   // still the render target
        REQUIRE(ris_test::read_pixel(0, 0) == ris_test::Pixel{255, 0, 0, 255});
        fu::delete_framebuffer(fb);
        tu::delete_texture(tex);
        ris::RenderTarget::unbind();
    }
    SECTION("multiple colour attachments") {
        gles::GlUint a = tu::create_texture_2d(4, 4, static_cast<const void*>(nullptr));
        gles::GlUint b = tu::create_texture_2d(4, 4, static_cast<const void*>(nullptr));
        const std::vector<gles::GlUint> textures = {a, b};
        gles::GlUint fb = fu::create(textures);
        REQUIRE(fb != 0);
        fu::delete_framebuffer(fb);
        tu::delete_texture(a);
        tu::delete_texture(b);
    }
    SECTION("depth buffers") {
        gles::GlUint color = tu::create_texture_2d(8, 8, static_cast<const void*>(nullptr));
        gles::GlUint rb = gles::renderbuffer_utilities::create(TextureFormat::Depth24Stencil8, 8, 8);
        gles::GlUint with_rb = fu::create_with_renderbuffer_as_depth_buffer(color, gles::DepthAttachmentType::DepthStencil, rb);
        REQUIRE(with_rb != 0);

        gles::GlUint depth_tex = tu::create_texture_2d(8, 8, static_cast<const void*>(nullptr), TextureFormat::Depth32Float);
        gles::GlUint with_tex = fu::create_with_texture_as_depth_buffer(color, gles::DepthAttachmentType::Depth, depth_tex);
        REQUIRE(with_tex != 0);

        // the attach functions on a colour-only framebuffer
        gles::GlUint plain = fu::create(color);
        REQUIRE_NOTHROW(fu::attach_depth_stencil_texture(plain, depth_tex, TextureFormat::Depth32Float));
        gles::GlUint plain2 = fu::create(color);
        REQUIRE_NOTHROW(fu::attach_depth_stencil_renderbuffer(plain2, rb, TextureFormat::Depth24Stencil8));
        REQUIRE_THROWS_AS(fu::attach_depth_stencil_renderbuffer(plain2, rb, TextureFormat::Rgba8Unorm), std::domain_error);
        REQUIRE_THROWS_AS(fu::attach_depth_stencil_texture(plain2, depth_tex, TextureFormat::Astc4x4Rgba), std::domain_error);

        REQUIRE(fu::depth_attachment_type_for(TextureFormat::Depth32Float) == gles::DepthAttachmentType::Depth);
        REQUIRE(fu::depth_attachment_type_for(TextureFormat::Depth24Stencil8) == gles::DepthAttachmentType::DepthStencil);
        REQUIRE_THROWS_AS(fu::depth_attachment_type_for(TextureFormat::Rgba8Unorm), std::domain_error);

        for (gles::GlUint* fb : {&with_rb, &with_tex, &plain, &plain2}) fu::delete_framebuffer(*fb);
        gles::renderbuffer_utilities::delete_renderbuffer(rb);
        tu::delete_texture(depth_tex);
        tu::delete_texture(color);
    }
    SECTION("invalid arguments and incomplete framebuffers") {
        REQUIRE_THROWS_AS(fu::create(std::span<const gles::GlUint>{}), std::invalid_argument);
        REQUIRE_THROWS_AS(fu::create(gles::GlUint{0}), std::invalid_argument);
        // a colour texture attached as depth is incomplete
        gles::GlUint color = tu::create_texture_2d(8, 8, static_cast<const void*>(nullptr));
        REQUIRE_THROWS_AS(fu::create_with_texture_as_depth_buffer(color, gles::DepthAttachmentType::Depth, color), std::runtime_error);
        REQUIRE_NOTHROW(gles::error_utilities::check_error("no stray error"));
        tu::delete_texture(color);
    }
}

TEST_CASE("render target on the gles utilities") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();

    ris::RenderTarget target(16, 16, true);   // with a depth stencil render buffer
    target.bind();
    ris::gfx::clear(0.0f, 0.0f, 1.0f, 1.0f);
    REQUIRE(ris_test::read_pixel(8, 8) == ris_test::Pixel{0, 0, 255, 255});
    ris::RenderTarget::unbind();
}

TEST_CASE("sampler utilities") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    namespace su = ris::gles::sampler_utilities;

    SECTION("defaults") {
        gles::GlUint sampler = su::create();
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
        gles::GlUint sampler = su::create(params, "shadow sampler");
        REQUIRE(sampler != 0);
        REQUIRE_NOTHROW(gles::error_utilities::check_error("sampler"));
        su::delete_sampler(sampler);
    }
}
