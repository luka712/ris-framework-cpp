// Tests of GlesTextureFactory / ATextureFactory (need a display). Textures are read back through a framebuffer.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

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
std::array<int, 4> texel(const ris::ATexture2D& texture, int x, int y) {
    uint32_t fb = fu::create(static_cast<uint32_t>(texture.getHandle()));
    fu::bind(fb);
    const std::array<int, 4> px = ris_test::read_pixel(x, y);
    fu::bind(0);
    fu::delete_framebuffer(fb);
    return px;
}

struct KtxFile {
    ktxTexture2* tex = nullptr;
    /// Loads the file, or holds nothing for a null path (the test creates the texture itself).
    explicit KtxFile(const char* path) {
        if (!path) return;
        REQUIRE(ris_ktxTexture2_CreateFromNamedFile(path, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &tex) == KTX_SUCCESS);
    }
    KtxFile(const KtxFile&) = delete;
    KtxFile& operator=(const KtxFile&) = delete;
    ~KtxFile() {
        if (tex) ris_ktxTexture2_Destroy(tex);
    }
};

} // namespace

TEST_CASE("texture factory createEmpty") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;
    const gles::GlesTextureFactory factory;

    SECTION("without a color the texture is transparent black") {
        const std::unique_ptr<ris::gles::GlesTexture2D> tex(factory.createEmpty(4, 2));
        REQUIRE(tex != nullptr);
        REQUIRE(tex->getWidth() == 4);
        REQUIRE(tex->getHeight() == 2);
        REQUIRE(tex->getMipLevels() == 1);
        REQUIRE(tex->getGlTexture() != 0);
        REQUIRE(texel(*tex, 3, 1) == std::array<int, 4>{0, 0, 0, 0});
    }
    SECTION("the default color fills every pixel") {
        const std::unique_ptr<ris::gles::GlesTexture2D> tex(factory.createEmpty(4, 4, ris::Color(1.0f, 0.5f, 0.0f, 1.0f), ris::TextureUsage::CopyDst |
                                             ris::TextureUsage::TextureBinding, ris::TextureFormat::Rgba8Unorm, "orange"));
        REQUIRE(texel(*tex, 0, 0) == std::array<int, 4>{255, 128, 0, 255});
        REQUIRE(texel(*tex, 3, 3) == std::array<int, 4>{255, 128, 0, 255});
    }
    SECTION("black is a color too") {
        const std::unique_ptr<ris::gles::GlesTexture2D> tex(factory.createEmpty(2, 2, ris::Color::black()));
        REQUIRE(texel(*tex, 1, 1) == std::array<int, 4>{0, 0, 0, 255});
    }
    SECTION("the sRGB format is accepted") {
        const std::unique_ptr<ris::gles::GlesTexture2D> tex(factory.createEmpty(2, 2, ris::Color::white(), ris::TextureUsage::TextureBinding,
                                             ris::TextureFormat::Rgba8UnormSrgb));
        REQUIRE(tex->getGlTexture() != 0);
    }
    SECTION("bad arguments") {
        REQUIRE_THROWS_AS(([&] { factory.createEmpty(0, 4); })(), std::invalid_argument);
        REQUIRE_THROWS_AS(([&] { factory.createEmpty(4, -1); })(), std::invalid_argument);
        REQUIRE_THROWS_AS(([&] {
                    factory.createEmpty(4, 4, std::nullopt, ris::TextureUsage::StorageBinding);
                })(), std::domain_error);
        REQUIRE_THROWS_AS(([&] {
                    factory.createEmpty(4, 4, std::nullopt, ris::TextureUsage::TextureBinding,
                                        ris::TextureFormat::Etc2Rgba8Unorm);   // compressed: needs data
                })(), std::domain_error);
        REQUIRE_THROWS_MATCHES(([&] { factory.createEmpty(0, 4); })(), std::invalid_argument,
                               Catch::Matchers::Message("Width and height must be greater than 0."));
    }
    SECTION("fast path: transparent black and no color make the same texture, nothing is uploaded") {
        const std::unique_ptr<ris::gles::GlesTexture2D> none(factory.createEmpty(4, 4));
        const std::unique_ptr<ris::gles::GlesTexture2D> zero(factory.createEmpty(4, 4, ris::Color::transparent()));
        REQUIRE(texel(*none, 2, 2) == std::array<int, 4>{0, 0, 0, 0});
        REQUIRE(texel(*zero, 2, 2) == std::array<int, 4>{0, 0, 0, 0});
    }
    SECTION("the colored path writes every pixel of the whole size") {
        const std::unique_ptr<ris::gles::GlesTexture2D> tex(factory.createEmpty(8, 2, ris::Color(0.0f, 1.0f, 0.0f, 0.5f)));
        REQUIRE(texel(*tex, 0, 0) == std::array<int, 4>{0, 255, 0, 128});
        REQUIRE(texel(*tex, 7, 1) == std::array<int, 4>{0, 255, 0, 128});
    }
    SECTION("an undefined format is Rgba8Unorm, the given format is kept") {
        const std::unique_ptr<ris::gles::GlesTexture2D> preferred(factory.createEmpty(2, 2));
        REQUIRE(preferred->getTextureFormat() == ris::TextureFormat::Rgba8Unorm);
        const std::unique_ptr<ris::gles::GlesTexture2D> srgb(factory.createEmpty(2, 2, std::nullopt, ris::TextureUsage::TextureBinding,
                                                                     ris::TextureFormat::Rgba8UnormSrgb));
        REQUIRE(srgb->getTextureFormat() == ris::TextureFormat::Rgba8UnormSrgb);
    }
    SECTION("useMipmap allocates the whole chain, with and without a color") {
        const std::unique_ptr<ris::gles::GlesTexture2D> empty(factory.createEmpty(4, 4, std::nullopt, ris::TextureUsage::TextureBinding,
                                                                      ris::TextureFormat::Undefined, "mips", true));
        REQUIRE(empty->getMipLevels() == 3);
        REQUIRE(empty->getSize() == (16 + 4 + 1) * 4);
        const std::unique_ptr<ris::gles::GlesTexture2D> colored(factory.createEmpty(4, 4, ris::Color::red(), ris::TextureUsage::TextureBinding,
                                                                        ris::TextureFormat::Undefined, "mips", true));
        REQUIRE(colored->getMipLevels() == 3);
        REQUIRE(texel(*colored, 1, 1) == std::array<int, 4>{255, 0, 0, 255});
    }
}

TEST_CASE("texture factory create") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;
    const gles::GlesTextureFactory factory;

    std::vector<uint8_t> pixels;   // 4x4: red on the left half, blue on the right half
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            if (x < 2) pixels.insert(pixels.end(), {255, 0, 0, 255});
            else pixels.insert(pixels.end(), {0, 0, 255, 128});
        }
    }

    SECTION("RGBA data without mip maps") {
        const std::unique_ptr<ris::gles::GlesTexture2D> tex(factory.create(4, 4, pixels, 4, "two colors"));
        REQUIRE(tex->getWidth() == 4);
        REQUIRE(tex->getHeight() == 4);
        REQUIRE(tex->getMipLevels() == 1);
        REQUIRE(texel(*tex, 0, 0) == std::array<int, 4>{255, 0, 0, 255});
        REQUIRE(texel(*tex, 3, 3) == std::array<int, 4>{0, 0, 255, 128});
    }
    SECTION("RGBA data with mip maps") {
        const std::unique_ptr<ris::gles::GlesTexture2D> tex(factory.create(4, 4, pixels, 4, "mips", ris::TextureUsage::CopyDst | ris::TextureUsage::TextureBinding,
                                        ris::TextureFormat::Rgba8Unorm, true));
        REQUIRE(tex->getMipLevels() == 3);
        REQUIRE(texel(*tex, 1, 1) == std::array<int, 4>{255, 0, 0, 255});
        REQUIRE_NOTHROW(gles::error_utilities::check_error("create with mips"));
    }
    SECTION("sRGB format") {
        const std::unique_ptr<ris::gles::GlesTexture2D> tex(factory.create(4, 4, pixels, 4, {}, ris::TextureUsage::TextureBinding, ris::TextureFormat::Rgba8UnormSrgb, true));
        REQUIRE(tex->getMipLevels() == 3);
    }
    SECTION("the TypeScript error messages") {
        REQUIRE_THROWS_MATCHES(([&] { factory.create(4, 4, {}); })(), std::invalid_argument,
                               Catch::Matchers::Message("Data must not be null."));
        REQUIRE_THROWS_MATCHES(([&] { factory.create(0, 4, pixels); })(), std::invalid_argument,
                               Catch::Matchers::Message("width and height must be greater than 0."));
        REQUIRE_THROWS_MATCHES(([&] { factory.create(4, 4, pixels, 0); })(), std::invalid_argument,
                               Catch::Matchers::Message("channels must be greater than 0."));
    }
    SECTION("the default format is Rgba8Unorm and the label and usage are kept") {
        const std::unique_ptr<ris::gles::GlesTexture2D> tex(factory.create(4, 4, pixels, 4, "kept", ris::TextureUsage::TextureBinding));
        REQUIRE(tex->getTextureFormat() == ris::TextureFormat::Rgba8Unorm);
        REQUIRE(tex->getLabel() == "kept");
        REQUIRE(tex->getTextureUsage() == ris::TextureUsage::TextureBinding);
        REQUIRE(tex->getSize() == 64);
        REQUIRE(tex->getTextureState() == ris::State::Initialized);
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
    ris_test::HiddenWindow window;
    const gles::GlesTextureFactory factory;
    const char* path = RIS_TEST_DATA_DIR "/test_basis_uastc.ktx2";

    SECTION("a Basis Universal file is transcoded to the best GPU format") {
        KtxFile file(path);
        const uint32_t width = file.tex->baseWidth, height = file.tex->baseHeight, levels = file.tex->numLevels;
        const std::unique_ptr<ris::ATexture2D> tex(factory.createFromKtx2(file.tex));
        REQUIRE(tex != nullptr);
        REQUIRE(tex->getWidth() == static_cast<int32_t>(width));
        REQUIRE(tex->getHeight() == static_cast<int32_t>(height));
        REQUIRE(tex->getMipLevels() == levels);
        REQUIRE(tex->getHandle() != 0);
    }
    SECTION("an explicit RGBA8 target gives readable pixels") {
        KtxFile file(path);
        ris::TextureDescriptor descriptor;
        descriptor.textureFormat = ris::TextureFormat::Rgba8Unorm;
        descriptor.label = "ktx rgba8";
        const std::unique_ptr<ris::ATexture2D> tex(factory.createFromKtx2(file.tex, descriptor));
        const std::array<int, 4> px = texel(*tex, tex->getWidth() / 2, tex->getHeight() / 2);
        REQUIRE(px[3] == 255);
        REQUIRE_NOTHROW(gles::error_utilities::check_error("ktx2 rgba8"));
    }
    SECTION("RGBA8 target with generated mip maps") {
        KtxFile file(path);
        ris::TextureDescriptor descriptor;
        descriptor.textureFormat = ris::TextureFormat::Rgba8Unorm;
        descriptor.generateMipmaps = true;
        const std::unique_ptr<ris::ATexture2D> tex(factory.createFromKtx2(file.tex, descriptor));
        const uint32_t full = GlesUtilities::texture().fullMipCount(tex->getWidth(), tex->getHeight());
        // levels stored in the file are used as they are, mip maps are only generated for a single level
        REQUIRE(tex->getMipLevels() == (file.tex->numLevels == 1 ? full : file.tex->numLevels));
    }
    SECTION("the default format is the first one the GPU supports: BC7, ASTC 4x4, BC3, ETC2") {
        const gles::GlesGraphicsDeviceFeatures features;
        ris::TextureFormat expected = ris::TextureFormat::Etc2Rgba8Unorm;
        if (features.supportsTextureCompressionBC()) expected = ris::TextureFormat::Bc7RgbaUnorm;
        else if (features.supportsTextureCompressionASTC()) expected = ris::TextureFormat::Astc4x4Rgba;
        else if (features.supportsTextureCompressionS3TC()) expected = ris::TextureFormat::Bc3RgbaUnorm;
        KtxFile file(path);
        const std::unique_ptr<ris::ATexture2D> tex(factory.createFromKtx2(file.tex));
        REQUIRE(tex->getTextureFormat() == expected);
        REQUIRE(tex->getSize() > 0);
        REQUIRE_NOTHROW(gles::error_utilities::check_error("ktx2 default format"));
    }
    SECTION("an explicit compressed format is honoured, or refused when the GPU lacks it") {
        const gles::GlesGraphicsDeviceFeatures features;
        const struct { ris::TextureFormat format; bool supported; } formats[] = {
            {ris::TextureFormat::Bc7RgbaUnorm, features.supportsTextureCompressionBC()},
            {ris::TextureFormat::Bc3RgbaUnorm, features.supportsTextureCompressionS3TC()},
            {ris::TextureFormat::Etc2Rgba8Unorm, features.supportsTextureCompressionETC2()},
            {ris::TextureFormat::Astc4x4Rgba, features.supportsTextureCompressionASTC()}};
        for (const auto& entry : formats) {
            KtxFile file(path);
            const uint32_t levels = file.tex->numLevels;
            ris::TextureDescriptor descriptor;
            descriptor.textureFormat = entry.format;
            if (entry.supported) {
                const std::unique_ptr<ris::ATexture2D> tex(factory.createFromKtx2(file.tex, descriptor));
                CHECK(tex->getTextureFormat() == entry.format);
                CHECK(tex->getMipLevels() == levels);
                CHECK(tex->getSize() > 0);
                CHECK_NOTHROW(gles::error_utilities::check_error("ktx2 explicit format"));
            } else {
                CHECK_THROWS_AS(factory.createFromKtx2(file.tex, descriptor), std::domain_error);
            }
        }
    }
    SECTION("a file that is not Basis Universal keeps its RGBA8 format, without transcoding") {
        ris_ktxTextureCreateInfo info{};
        info.vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
        info.baseWidth = 2;
        info.baseHeight = 2;
        info.baseDepth = 1;
        info.numDimensions = 2;
        info.numLevels = 1;
        info.numLayers = 1;
        info.numFaces = 1;
        KtxFile file(nullptr);
        REQUIRE(ris_ktxTexture2_Create(&info, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &file.tex) == KTX_SUCCESS);
        const uint8_t pixels[16] = {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 10, 20, 30, 40};
        REQUIRE(ris_ktxTexture2_SetImageFromMemory(file.tex, 0, 0, 0, pixels, sizeof(pixels)) == KTX_SUCCESS);
        const std::unique_ptr<ris::ATexture2D> tex(factory.createFromKtx2(file.tex));
        CHECK(tex->getTextureFormat() == ris::TextureFormat::Rgba8Unorm);
        CHECK(tex->getMipLevels() == 1);
        CHECK(tex->getSize() == 16);
        CHECK(texel(*tex, 0, 0) == std::array<int, 4>{255, 0, 0, 255});
        CHECK(texel(*tex, 1, 1) == std::array<int, 4>{10, 20, 30, 40});
    }
    SECTION("a Vulkan format that TextureFormat does not describe is refused") {
        ris_ktxTextureCreateInfo info{};
        info.vkFormat = VK_FORMAT_R8_UNORM;
        info.baseWidth = 2;
        info.baseHeight = 2;
        info.baseDepth = 1;
        info.numDimensions = 2;
        info.numLevels = 1;
        info.numLayers = 1;
        info.numFaces = 1;
        KtxFile file(nullptr);
        REQUIRE(ris_ktxTexture2_Create(&info, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &file.tex) == KTX_SUCCESS);
        CHECK_THROWS_AS(factory.createFromKtx2(file.tex), std::domain_error);
    }
    SECTION("bad arguments") {
        KtxFile file(path);
        REQUIRE_THROWS_AS(([&] { factory.createFromKtx2(nullptr); })(), std::invalid_argument);
        ris::TextureDescriptor storage;
        storage.textureUsage = ris::TextureUsage::StorageBinding;
        REQUIRE_THROWS_AS(([&] { factory.createFromKtx2(file.tex, storage); })(), std::domain_error);
        ris::TextureDescriptor depth;
        depth.textureFormat = ris::TextureFormat::Depth32Float;   // no transcode target
        REQUIRE_THROWS_AS(([&] { factory.createFromKtx2(file.tex, depth); })(), std::domain_error);
    }
}

TEST_CASE("a factory made with a framework asks its graphics device for the GPU features") {
    if (!ris_test::display_available()) SKIP("no display available");
    RisFrameworkConfiguration config = ris_createFrameworkConfiguration();
    ris::Framework framework(config);
    const gles::GlesTextureFactory factory(framework);

    const gles::GlesGraphicsDeviceFeatures& features =
        dynamic_cast<const gles::GlesGraphicsDeviceFeatures&>(framework.graphicsDevice().getFeatures());
    ris::TextureFormat expected = ris::TextureFormat::Etc2Rgba8Unorm;
    if (features.supportsTextureCompressionBC()) expected = ris::TextureFormat::Bc7RgbaUnorm;
    else if (features.supportsTextureCompressionASTC()) expected = ris::TextureFormat::Astc4x4Rgba;
    else if (features.supportsTextureCompressionS3TC()) expected = ris::TextureFormat::Bc3RgbaUnorm;

    KtxFile file(RIS_TEST_DATA_DIR "/test_basis_uastc.ktx2");
    const std::unique_ptr<ris::ATexture2D> tex(factory.createFromKtx2(file.tex));
    REQUIRE(tex->getTextureFormat() == expected);
}

TEST_CASE("Framework::textureFactory is the OpenGL ES factory") {
    if (!ris_test::display_available()) SKIP("no display available");
    RisFrameworkConfiguration config = ris_createFrameworkConfiguration();
    ris::Framework framework(config);
    REQUIRE(dynamic_cast<gles::GlesTextureFactory*>(&framework.textureFactory()) != nullptr);
    const std::unique_ptr<ris::ATexture2D> tex(framework.textureFactory().createEmpty(2, 2, ris::Color::red()));
    REQUIRE(tex->getWidth() == 2);
}
