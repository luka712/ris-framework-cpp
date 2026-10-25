// Tests of TextureUtilities, the neutral texture helpers (no display needed).
#include <catch2/catch_test_macros.hpp>

#include <ris/rendering/texture/texture_utilities.hpp>

using ris::TextureFormat;
using ris::TextureUtilities;

TEST_CASE("TextureUtilities isCompressedTextureFormat") {
    CHECK(TextureUtilities::isCompressedTextureFormat(TextureFormat::Etc2Rgba8Unorm));
    CHECK(TextureUtilities::isCompressedTextureFormat(TextureFormat::Astc4x4Rgba));
    CHECK(TextureUtilities::isCompressedTextureFormat(TextureFormat::Bc3RgbaUnorm));
    CHECK(TextureUtilities::isCompressedTextureFormat(TextureFormat::Bc7RgbaUnorm));
    CHECK_FALSE(TextureUtilities::isCompressedTextureFormat(TextureFormat::Rgba8Unorm));
    CHECK_FALSE(TextureUtilities::isCompressedTextureFormat(TextureFormat::Depth32Float));
    CHECK_FALSE(TextureUtilities::isCompressedTextureFormat(TextureFormat::Undefined));
}

TEST_CASE("TextureUtilities mipLevels") {
    CHECK(TextureUtilities::mipLevels(1, 1) == 1);
    CHECK(TextureUtilities::mipLevels(2, 2) == 2);
    CHECK(TextureUtilities::mipLevels(4, 4) == 3);
    CHECK(TextureUtilities::mipLevels(8, 2) == 4);
    CHECK(TextureUtilities::mipLevels(5, 3) == 3);     // floor(log2(5)) + 1
    CHECK(TextureUtilities::mipLevels(256, 256) == 9);
    CHECK(TextureUtilities::mipLevels(0, 0) == 1);
}

TEST_CASE("TextureUtilities getVRamSize") {
    CHECK(TextureUtilities::getVRamSize(TextureFormat::Rgba8Unorm, 4, 4) == 64);
    CHECK(TextureUtilities::getVRamSize(TextureFormat::Rgba8UnormSrgb, 3, 1) == 12);
    CHECK(TextureUtilities::getVRamSize(TextureFormat::Depth24Stencil8, 2, 2) == 16);
    // compressed: whole blocks of 4x4 pixels, 16 bytes each
    CHECK(TextureUtilities::getVRamSize(TextureFormat::Etc2Rgba8Unorm, 8, 8) == 64);
    CHECK(TextureUtilities::getVRamSize(TextureFormat::Etc2Rgba8Unorm, 1, 1) == 16);
    CHECK(TextureUtilities::getVRamSize(TextureFormat::Astc4x4Rgba, 5, 4) == 32);
    CHECK(TextureUtilities::getVRamSize(TextureFormat::Undefined, 4, 4) == 0);
    CHECK(TextureUtilities::getVRamSize(TextureFormat::Rgba8Unorm, 0, 4) == 0);
}
