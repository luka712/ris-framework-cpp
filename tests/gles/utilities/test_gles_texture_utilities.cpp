// Tests of GlesTextureUtilities (need a display). Textures are read back through a framebuffer.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

#include <stdexcept>
#include <vector>

#include <ris/gles/gles.hpp>

#include "../gles_test_utils.hpp"

namespace gles = ris::gles;
namespace fu = ris::gles::framebuffer_utilities;
using ris::TextureFormat;

namespace {

std::vector<uint8_t> solid(int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    std::vector<uint8_t> px;
    for (int i = 0; i < w * h; ++i) px.insert(px.end(), {r, g, b, a});
    return px;
}

/// Reads the pixel (x, y) of the texture through a temporary framebuffer.
std::array<int, 4> texel(uint32_t texture, int x, int y) {
    uint32_t fb = fu::create(texture);
    fu::bind(fb);
    const std::array<int, 4> px = ris_test::read_pixel(x, y);
    fu::bind(0);
    fu::delete_framebuffer(fb);
    return px;
}

} // namespace

TEST_CASE("GlesTextureUtilities without a window throws std::logic_error") {
    const std::vector<uint8_t> block(16, 0);
    const std::vector<std::span<const uint8_t>> levels = {block};
    REQUIRE_THROWS_AS(GlesUtilities::texture().createTexture2D(4, 4), std::logic_error);
    REQUIRE_THROWS_AS(GlesUtilities::texture().createCompressedTexture2D(4, 4, 4, 4, levels), std::logic_error);
    // deleting needs no window
    uint32_t none = 7;
    GlesUtilities::texture().deleteTexture(none);
    REQUIRE(none == 0);
}

TEST_CASE("GlesTextureUtilities createTexture2D") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;

    SECTION("mip count") {
        REQUIRE(GlesUtilities::texture().fullMipCount(1, 1) == 1);
        REQUIRE(GlesUtilities::texture().fullMipCount(2, 1) == 2);
        REQUIRE(GlesUtilities::texture().fullMipCount(256, 256) == 9);
        REQUIRE(GlesUtilities::texture().fullMipCount(300, 20) == 9);
    }
    SECTION("pixels are uploaded") {
        const auto red = solid(4, 4, 255, 0, 0, 255);
        uint32_t tex = GlesUtilities::texture().createTexture2D(4, 4, red.data(), TextureFormat::Rgba8Unorm, false, 1.0f, "red");
        REQUIRE(tex != 0);
        REQUIRE(texel(tex, 2, 2) == std::array<int, 4>{255, 0, 0, 255});
        GlesUtilities::texture().deleteTexture(tex);
        REQUIRE(tex == 0);
    }
    SECTION("mip levels are uploaded one by one") {
        const auto level0 = solid(4, 4, 255, 0, 0, 255);
        const auto level1 = solid(2, 2, 0, 255, 0, 255);
        const auto level2 = solid(1, 1, 0, 0, 255, 255);
        const std::vector<std::span<const uint8_t>> levels = {level0, level1, level2};
        uint32_t tex = GlesUtilities::texture().createTexture2D(4, 4, levels);
        REQUIRE(tex != 0);
        REQUIRE_NOTHROW(gles::error_utilities::check_error("mips"));
        GlesUtilities::texture().deleteTexture(tex);
    }
    SECTION("an empty level is left uninitialised") {
        const auto level0 = solid(2, 2, 1, 2, 3, 4);
        const std::vector<std::span<const uint8_t>> levels = {level0, std::span<const uint8_t>{}};
        uint32_t tex = GlesUtilities::texture().createTexture2D(2, 2, levels);
        REQUIRE(texel(tex, 0, 0) == std::array<int, 4>{1, 2, 3, 4});
        GlesUtilities::texture().deleteTexture(tex);
    }
    SECTION("generated mip maps and uninitialised textures") {
        const auto px = solid(8, 8, 9, 9, 9, 255);
        uint32_t mips = GlesUtilities::texture().createTexture2D(8, 8, px.data(), TextureFormat::Rgba8UnormSrgb, true);
        REQUIRE(mips != 0);
        GlesUtilities::texture().deleteTexture(mips);
        uint32_t blank = GlesUtilities::texture().createTexture2D(8, 8, static_cast<const void*>(nullptr));
        REQUIRE(blank != 0);
        GlesUtilities::texture().deleteTexture(blank);
    }
    SECTION("depth textures") {
        uint32_t depth = GlesUtilities::texture().createTexture2D(8, 8, static_cast<const void*>(nullptr), TextureFormat::Depth32Float);
        REQUIRE(depth != 0);
        GlesUtilities::texture().deleteTexture(depth);
        REQUIRE_THROWS_AS(
            GlesUtilities::texture().createTexture2D(8, 8, static_cast<const void*>(nullptr), TextureFormat::Depth32Float, true), std::invalid_argument);
    }
    SECTION("invalid arguments") {
        const auto small = solid(1, 1, 0, 0, 0, 0);
        REQUIRE_THROWS_AS(GlesUtilities::texture().createTexture2D(0, 4, static_cast<const void*>(nullptr)), std::invalid_argument);
        REQUIRE_THROWS_AS(GlesUtilities::texture().createTexture2D(4, -1, static_cast<const void*>(nullptr)), std::invalid_argument);
        REQUIRE_THROWS_AS(GlesUtilities::texture().createTexture2D(4, 4, static_cast<const void*>(nullptr), TextureFormat::Astc4x4Rgba), std::domain_error);
        REQUIRE_THROWS_AS(GlesUtilities::texture().createTexture2D(4, 4, static_cast<const void*>(nullptr), TextureFormat::Red32Float), std::domain_error);
        const std::vector<std::span<const uint8_t>> too_small = {small};
        REQUIRE_THROWS_AS(GlesUtilities::texture().createTexture2D(4, 4, too_small), std::invalid_argument);
        const std::vector<std::span<const uint8_t>> too_many = {small, small, small, small};
        REQUIRE_THROWS_AS(GlesUtilities::texture().createTexture2D(4, 4, too_many), std::invalid_argument);   // only 3 levels fit
        REQUIRE_NOTHROW(gles::error_utilities::check_error("no stray error"));
    }
}

TEST_CASE("GlesTextureUtilities createCompressedTexture2D") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;

    // ETC2 RGBA8 is core in OpenGL ES 3 (and emulated by ANGLE): 16 bytes per 4x4 block.
    const std::vector<uint8_t> block(16, 0);
    SECTION("a multiple of the block size uses immutable storage") {
        const std::vector<uint8_t> level0(16 * 4, 0);   // 8x8 pixels = 4 blocks
        const std::vector<std::span<const uint8_t>> levels = {level0, block, block, block};
        uint32_t tex = GlesUtilities::texture().createCompressedTexture2D(8, 8, 4, 4, levels, TextureFormat::Etc2Rgba8Unorm);
        REQUIRE(tex != 0);
        GlesUtilities::texture().deleteTexture(tex);
    }
    SECTION("other sizes are uploaded rounded up to whole blocks") {
        const std::vector<std::span<const uint8_t>> levels = {block};   // 3x3 -> one block
        uint32_t tex = GlesUtilities::texture().createCompressedTexture2D(3, 3, 4, 4, levels, TextureFormat::Etc2Rgba8Unorm);
        REQUIRE(tex != 0);
        GlesUtilities::texture().deleteTexture(tex);
    }
    SECTION("invalid arguments") {
        const std::vector<std::span<const uint8_t>> levels = {block};
        REQUIRE_THROWS_AS(GlesUtilities::texture().createCompressedTexture2D(4, 4, 4, 4, {}, TextureFormat::Etc2Rgba8Unorm), std::invalid_argument);
        REQUIRE_THROWS_AS(GlesUtilities::texture().createCompressedTexture2D(4, 4, 4, 4, levels, TextureFormat::Rgba8Unorm), std::domain_error);
        REQUIRE_THROWS_AS(GlesUtilities::texture().createCompressedTexture2D(0, 4, 4, 4, levels, TextureFormat::Etc2Rgba8Unorm), std::invalid_argument);
        REQUIRE_THROWS_AS(GlesUtilities::texture().createCompressedTexture2D(4, 4, 0, 4, levels, TextureFormat::Etc2Rgba8Unorm), std::invalid_argument);
        const std::vector<std::span<const uint8_t>> empty_level = {std::span<const uint8_t>{}};
        REQUIRE_THROWS_AS(GlesUtilities::texture().createCompressedTexture2D(4, 4, 4, 4, empty_level, TextureFormat::Etc2Rgba8Unorm), std::invalid_argument);
        // a block that is too short for the size makes GL fail: reported as Error, nothing leaks, no stray error
        const std::vector<uint8_t> short_block(4, 0);
        const std::vector<std::span<const uint8_t>> short_levels = {short_block};
        REQUIRE_THROWS_AS(GlesUtilities::texture().createCompressedTexture2D(4, 4, 4, 4, short_levels, TextureFormat::Etc2Rgba8Unorm), std::runtime_error);
        REQUIRE_NOTHROW(gles::error_utilities::check_error("no stray error"));
    }
}

TEST_CASE("GlesTextureUtilities mip maps") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;

    SECTION("generated from level 0") {
        // 8x8 with a generated chain (8, 4, 2, 1): level 0 keeps its pixels and GL reports no error.
        const auto px = solid(8, 8, 200, 100, 50, 255);
        uint32_t tex = GlesUtilities::texture().createTexture2D(8, 8, px.data(), TextureFormat::Rgba8Unorm, true);
        REQUIRE(tex != 0);
        CHECK(texel(tex, 5, 5) == std::array<int, 4>{200, 100, 50, 255});
        REQUIRE_NOTHROW(gles::error_utilities::check_error("generated mips"));
        GlesUtilities::texture().deleteTexture(tex);
    }
    SECTION("provided levels are used as they are") {
        const auto level0 = solid(4, 4, 255, 0, 0, 255);
        const auto level1 = solid(2, 2, 0, 255, 0, 255);
        const auto level2 = solid(1, 1, 0, 0, 255, 255);
        const std::vector<std::span<const uint8_t>> levels = {level0, level1, level2};
        // useMipMaps is ignored when more than one level is given: the levels stay as they are.
        uint32_t tex = GlesUtilities::texture().createTexture2D(4, 4, levels, TextureFormat::Rgba8Unorm, true);
        REQUIRE(tex != 0);
        CHECK(texel(tex, 3, 3) == std::array<int, 4>{255, 0, 0, 255});
        REQUIRE_NOTHROW(gles::error_utilities::check_error("provided mips"));
        GlesUtilities::texture().deleteTexture(tex);
    }
}

TEST_CASE("GlesTextureUtilities anisotropy above 1 is not implemented") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;

    // Like ris-framework-ts: throws std::runtime_error("Not implemented yet.") and creates nothing.
    const void* none = nullptr;
    REQUIRE_THROWS_MATCHES(GlesUtilities::texture().createTexture2D(4, 4, none, TextureFormat::Rgba8Unorm, false, 4.0f),
                           std::runtime_error, Catch::Matchers::Message("Not implemented yet."));
    const std::vector<uint8_t> block(16, 0);
    const std::vector<std::span<const uint8_t>> levels = {block};
    REQUIRE_THROWS_MATCHES(GlesUtilities::texture().createCompressedTexture2D(4, 4, 4, 4, levels, TextureFormat::Etc2Rgba8Unorm, 4.0f),
                           std::runtime_error, Catch::Matchers::Message("Not implemented yet."));
    REQUIRE_NOTHROW(gles::error_utilities::check_error("anisotropy"));

    // 1 (the default) is fine
    uint32_t tex = GlesUtilities::texture().createTexture2D(4, 4, none, TextureFormat::Rgba8Unorm, false, 1.0f);
    REQUIRE(tex != 0);
    GlesUtilities::texture().deleteTexture(tex);
}
