// Tests of GlesTextureFactory / ATextureFactory (need a display). Textures are read back through a framebuffer.
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <memory>
#include <vector>

#include <ris/ris.hpp>

#if __has_include(<ris_ktx2/ris_ktx.hpp>)
#include <ris_ktx2/ris_ktx.hpp>
#else
#include <ris_ktx.hpp>
#endif

#include "../gles_test_utils.hpp"

namespace gles = ris::gles;
namespace fu = ris::gles::framebuffer_utilities;

namespace {

/// Reads the pixel (x, y) of the texture through a temporary framebuffer.
ris_test::Pixel texel(const ris::Texture& texture, int x, int y) {
    gles::GlUint fb = fu::create(texture.gl_handle());
    fu::bind(fb);
    const ris_test::Pixel px = ris_test::read_pixel(x, y);
    fu::bind(0);
    fu::delete_framebuffer(fb);
    return px;
}

struct KtxFile {
    ktxTexture2* tex = nullptr;
    explicit KtxFile(const char* path) {
        REQUIRE(ris_ktxTexture2_CreateFromNamedFile(path, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &tex) == KTX_SUCCESS);
    }
    KtxFile(const KtxFile&) = delete;
    KtxFile& operator=(const KtxFile&) = delete;
    ~KtxFile() {
        if (tex) ris_ktxTexture2_Destroy(tex);
    }
};

} // namespace

TEST_CASE("the default texture factory is the OpenGL ES one") {
    const std::unique_ptr<ris::ATextureFactory> factory = ris::create_default_texture_factory();
    REQUIRE(dynamic_cast<gles::GlesTextureFactory*>(factory.get()) != nullptr);
}

TEST_CASE("texture factory createEmpty") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    const gles::GlesTextureFactory factory;

    SECTION("without a color the texture is transparent black") {
        const std::unique_ptr<ris::Texture> tex(factory.createEmpty(4, 2));
        REQUIRE(tex != nullptr);
        REQUIRE(tex->width() == 4);
        REQUIRE(tex->height() == 2);
        REQUIRE(tex->mip_levels() == 1);
        REQUIRE(tex->gl_handle() != 0);
        REQUIRE(texel(*tex, 3, 1) == ris_test::Pixel{0, 0, 0, 0});
    }
    SECTION("the default color fills every pixel") {
        const std::unique_ptr<ris::Texture> tex(factory.createEmpty(4, 4, ris::Color(1.0f, 0.5f, 0.0f, 1.0f), ris::TextureUsage::CopyDst |
                                             ris::TextureUsage::TextureBinding, ris::TextureFormat::Rgba8Unorm, "orange"));
        REQUIRE(texel(*tex, 0, 0) == ris_test::Pixel{255, 128, 0, 255});
        REQUIRE(texel(*tex, 3, 3) == ris_test::Pixel{255, 128, 0, 255});
    }
    SECTION("black is a color too") {
        const std::unique_ptr<ris::Texture> tex(factory.createEmpty(2, 2, ris::Color::black()));
        REQUIRE(texel(*tex, 1, 1) == ris_test::Pixel{0, 0, 0, 255});
    }
    SECTION("the sRGB format is accepted") {
        const std::unique_ptr<ris::Texture> tex(factory.createEmpty(2, 2, ris::Color::white(), ris::TextureUsage::TextureBinding,
                                             ris::TextureFormat::Rgba8UnormSrgb));
        REQUIRE(tex->gl_handle() != 0);
    }
    SECTION("bad arguments") {
        REQUIRE_THROWS_AS(([&] { factory.createEmpty(0, 4); })(), std::invalid_argument);
        REQUIRE_THROWS_AS(([&] { factory.createEmpty(4, -1); })(), std::invalid_argument);
        REQUIRE_THROWS_AS(([&] {
                    factory.createEmpty(4, 4, std::nullopt, ris::TextureUsage::StorageBinding);
                })(), std::domain_error);
        REQUIRE_THROWS_AS(([&] {
                    factory.createEmpty(4, 4, std::nullopt, ris::TextureUsage::TextureBinding,
                                        ris::TextureFormat::Depth32Float);
                })(), std::domain_error);
    }
}

TEST_CASE("texture factory create") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    const gles::GlesTextureFactory factory;

    std::vector<uint8_t> pixels;   // 4x4: red on the left half, blue on the right half
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            if (x < 2) pixels.insert(pixels.end(), {255, 0, 0, 255});
            else pixels.insert(pixels.end(), {0, 0, 255, 128});
        }
    }

    SECTION("RGBA data without mip maps") {
        const std::unique_ptr<ris::Texture> tex(factory.create(4, 4, pixels, 4, "two colors"));
        REQUIRE(tex->width() == 4);
        REQUIRE(tex->height() == 4);
        REQUIRE(tex->mip_levels() == 1);
        REQUIRE(texel(*tex, 0, 0) == ris_test::Pixel{255, 0, 0, 255});
        REQUIRE(texel(*tex, 3, 3) == ris_test::Pixel{0, 0, 255, 128});
    }
    SECTION("RGBA data with mip maps") {
        const std::unique_ptr<ris::Texture> tex(factory.create(4, 4, pixels, 4, "mips", ris::TextureUsage::CopyDst | ris::TextureUsage::TextureBinding,
                                        ris::TextureFormat::Rgba8Unorm, true));
        REQUIRE(tex->mip_levels() == 3);
        REQUIRE(texel(*tex, 1, 1) == ris_test::Pixel{255, 0, 0, 255});
        REQUIRE_NOTHROW(gles::error_utilities::check_error("create with mips"));
    }
    SECTION("sRGB format") {
        const std::unique_ptr<ris::Texture> tex(factory.create(4, 4, pixels, 4, {}, ris::TextureUsage::TextureBinding, ris::TextureFormat::Rgba8UnormSrgb, true));
        REQUIRE(tex->mip_levels() == 3);
    }
    SECTION("bad arguments") {
        REQUIRE_THROWS_AS(([&] { factory.create(4, 4, {}); })(), std::invalid_argument);
        REQUIRE_THROWS_AS(([&] { factory.create(0, 4, pixels); })(), std::invalid_argument);
        REQUIRE_THROWS_AS(([&] { factory.create(4, 4, pixels, 0); })(), std::invalid_argument);
        REQUIRE_THROWS_AS(([&] { factory.create(4, 4, pixels, 3); })(), std::domain_error);
        REQUIRE_THROWS_AS(([&] { factory.create(2, 4, pixels); })(), std::invalid_argument);   // size mismatch
        REQUIRE_THROWS_AS(([&] {
                    factory.create(4, 4, pixels, 4, {}, ris::TextureUsage::StorageBinding);
                })(), std::domain_error);
        REQUIRE_THROWS_AS(([&] {
                    factory.create(4, 4, pixels, 4, {}, ris::TextureUsage::TextureBinding, ris::TextureFormat::Astc4x4Rgba);
                })(), std::domain_error);
    }
}

TEST_CASE("texture factory createFromKtx2") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    const std::unique_ptr<ris::ATextureFactory> factory = ris::create_default_texture_factory();
    const char* path = RIS_TEST_DATA_DIR "/test_basis_uastc.ktx2";

    SECTION("a Basis Universal file is transcoded to the best GPU format") {
        KtxFile file(path);
        const uint32_t width = file.tex->baseWidth, height = file.tex->baseHeight, levels = file.tex->numLevels;
        const std::unique_ptr<ris::Texture> tex(factory->createFromKtx2(file.tex));
        REQUIRE(tex != nullptr);
        REQUIRE(tex->width() == static_cast<int32_t>(width));
        REQUIRE(tex->height() == static_cast<int32_t>(height));
        REQUIRE(tex->mip_levels() == levels);
        REQUIRE(tex->gl_handle() != 0);
    }
    SECTION("an explicit RGBA8 target gives readable pixels") {
        KtxFile file(path);
        ris::TextureDescriptor descriptor;
        descriptor.textureFormat = ris::TextureFormat::Rgba8Unorm;
        descriptor.label = "ktx rgba8";
        const std::unique_ptr<ris::Texture> tex(factory->createFromKtx2(file.tex, descriptor));
        const ris_test::Pixel px = texel(*tex, tex->width() / 2, tex->height() / 2);
        REQUIRE(px[3] == 255);
        REQUIRE_NOTHROW(gles::error_utilities::check_error("ktx2 rgba8"));
    }
    SECTION("RGBA8 target with generated mip maps") {
        KtxFile file(path);
        ris::TextureDescriptor descriptor;
        descriptor.textureFormat = ris::TextureFormat::Rgba8Unorm;
        descriptor.generateMipmaps = true;
        const std::unique_ptr<ris::Texture> tex(factory->createFromKtx2(file.tex, descriptor));
        const uint32_t full = gles::texture_utilities::full_mip_count(tex->width(), tex->height());
        // levels stored in the file are used as they are, mip maps are only generated for a single level
        REQUIRE(tex->mip_levels() == (file.tex->numLevels == 1 ? full : file.tex->numLevels));
    }
    SECTION("bad arguments") {
        KtxFile file(path);
        REQUIRE_THROWS_AS(([&] { factory->createFromKtx2(nullptr); })(), std::invalid_argument);
        ris::TextureDescriptor storage;
        storage.textureUsage = ris::TextureUsage::StorageBinding;
        REQUIRE_THROWS_AS(([&] { factory->createFromKtx2(file.tex, storage); })(), std::domain_error);
        ris::TextureDescriptor bc7;
        bc7.textureFormat = ris::TextureFormat::Bc7RgbaUnorm;
        REQUIRE_THROWS_AS(([&] { factory->createFromKtx2(file.tex, bc7); })(), std::domain_error);
    }
}

TEST_CASE("Framework::textureFactory is the OpenGL ES factory") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::FrameworkConfiguration config;
    config.hidden = true;
    ris::Framework framework(config);
    REQUIRE(dynamic_cast<gles::GlesTextureFactory*>(&framework.textureFactory()) != nullptr);
    const std::unique_ptr<ris::Texture> tex(framework.textureFactory().createEmpty(2, 2, ris::Color::red()));
    REQUIRE(tex->width() == 2);
}
