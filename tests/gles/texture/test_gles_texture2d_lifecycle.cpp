// Tests of the WebGlTexture2D like life cycle of GlesTexture2D: construct from a descriptor, initialize(),
// dispose(), and the default texture of a framework (need a display).
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include <ris/ris.hpp>

#include "../gles_test_utils.hpp"

namespace gles = ris::gles;

namespace {

ris::TextureDescriptor describe(int32_t width, int32_t height, const std::vector<uint8_t>& pixels,
                                bool generateMipmaps = false) {
    ris::TextureDescriptor descriptor;
    descriptor.width = width;
    descriptor.height = height;
    if (!pixels.empty()) descriptor.data.emplace_back(pixels);
    descriptor.textureFormat = ris::TextureFormat::Rgba8Unorm;
    descriptor.label = "lifecycle";
    descriptor.generateMipmaps = generateMipmaps;
    return descriptor;
}

} // namespace

TEST_CASE("GlesTexture2D descriptor constructor needs a valid size and does nothing on the GPU") {
    // no window: the constructor must not touch GL
    const std::vector<uint8_t> pixels(2 * 2 * 4, 255);
    gles::GlesTexture2D tex(describe(2, 2, pixels));
    CHECK(tex.getTextureState() == ris::State::Created);
    CHECK(tex.getGlTexture() == 0);
    CHECK(tex.getHandle() == 0);
    CHECK(tex.getWidth() == 2);
    CHECK(tex.getLabel() == "lifecycle");
    CHECK(tex.getTextureFormat() == ris::TextureFormat::Rgba8Unorm);
    CHECK_THROWS_AS(tex.initialize(), std::logic_error);   // needs the window's context

    CHECK_THROWS_AS(gles::GlesTexture2D(describe(0, 2, pixels)), std::invalid_argument);
    CHECK_THROWS_AS(gles::GlesTexture2D(describe(2, -1, pixels)), std::invalid_argument);

    SECTION("an undefined format is the preferred one, Rgba8Unorm") {
        ris::TextureDescriptor descriptor = describe(2, 2, pixels);
        descriptor.textureFormat = ris::TextureFormat::Undefined;
        CHECK(gles::GlesTexture2D(descriptor).getTextureFormat() == ris::TextureFormat::Rgba8Unorm);
    }
}

TEST_CASE("GlesTexture2D initialize() creates the GL texture, size and mip levels") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();

    SECTION("plain texture") {
        const std::vector<uint8_t> pixels(4 * 4 * 4, 100);
        gles::GlesTexture2D tex(describe(4, 4, pixels));
        tex.initialize();
        CHECK(tex.getTextureState() == ris::State::Initialized);
        CHECK(tex.getGlTexture() != 0);
        CHECK(tex.getGlTexture() == tex.getHandle());
        CHECK(tex.getMipLevels() == 1);
        CHECK(tex.getSize() == 64);
        CHECK_NOTHROW(tex.getDefaultTextureView());
    }
    SECTION("generated mip maps count every level") {
        const std::vector<uint8_t> pixels(8 * 2 * 4, 100);
        gles::GlesTexture2D tex(describe(8, 2, pixels, true));
        tex.initialize();
        CHECK(tex.getMipLevels() == 4);
        CHECK(tex.getSize() == (16 + 4 + 2 + 1) * 4);
    }
    SECTION("provided mip levels") {
        const std::vector<uint8_t> level0(4 * 4 * 4, 1);
        const std::vector<uint8_t> level1(2 * 2 * 4, 2);
        ris::TextureDescriptor descriptor = describe(4, 4, level0);
        descriptor.data.emplace_back(level1);
        gles::GlesTexture2D tex(descriptor);
        tex.initialize();
        CHECK(tex.getMipLevels() == 2);
        CHECK(tex.getSize() == (16 + 4) * 4);
    }
    SECTION("without data the texture is uninitialised but complete") {
        gles::GlesTexture2D tex(describe(4, 4, {}));
        tex.initialize();
        CHECK(tex.getGlTexture() != 0);
    }
    SECTION("compressed formats go through createCompressedTexture2D") {
        const std::vector<uint8_t> blocks(4 * 16, 0);   // 8x8 ETC2 RGBA8: four blocks of 16 bytes
        ris::TextureDescriptor descriptor = describe(8, 8, blocks);
        descriptor.textureFormat = ris::TextureFormat::Etc2Rgba8Unorm;
        gles::GlesTexture2D tex(descriptor);
        tex.initialize();
        CHECK(tex.getTextureFormat() == ris::TextureFormat::Etc2Rgba8Unorm);
        CHECK(tex.getGlTexture() != 0);
        CHECK(tex.getMipLevels() == 1);
        CHECK(tex.getSize() == 64);

        ris::TextureDescriptor missing = describe(8, 8, {});
        missing.textureFormat = ris::TextureFormat::Etc2Rgba8Unorm;
        gles::GlesTexture2D bad(missing);
        CHECK_THROWS_AS(bad.initialize(), std::invalid_argument);   // compressed textures need data
        CHECK(bad.getTextureState() == ris::State::Created);
    }
    SECTION("initialize twice or after dispose throws with the TypeScript messages") {
        const std::vector<uint8_t> pixels(2 * 2 * 4, 255);
        gles::GlesTexture2D tex(describe(2, 2, pixels));
        tex.initialize();
        CHECK_THROWS_MATCHES(tex.initialize(), std::logic_error, Catch::Matchers::Message("Texture is already initialized."));
        tex.dispose();
        CHECK_THROWS_MATCHES(tex.initialize(), std::logic_error, Catch::Matchers::Message("Texture is already disposed."));
    }
    SECTION("a texture from the factory is a texture like any other") {
        const gles::GlesTextureFactory factory;
        const std::unique_ptr<gles::GlesTexture2D> tex(factory.createEmpty(4, 4));
        CHECK(tex->getGlTexture() != 0);
        CHECK(tex->getSize() == 64);
    }
}

TEST_CASE("GlesTexture2D dispose()") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    const std::vector<uint8_t> pixels(2 * 2 * 4, 255);

    SECTION("is idempotent and raises the event once") {
        int calls = 0;
        ris::State seen = ris::State::Created;
        {
            gles::GlesTexture2D tex(describe(2, 2, pixels));
            tex.initialize();
            tex.addOnDisposedListener([&](ris::ATexture2D& t) {
                ++calls;
                seen = t.getTextureState();
            });
            tex.dispose();
            CHECK(tex.getTextureState() == ris::State::Disposed);
            CHECK(tex.getGlTexture() == 0);
            CHECK(calls == 1);
            CHECK(seen == ris::State::Disposed);
            tex.dispose();
            CHECK(calls == 1);
        }   // the destructor disposes again: still once
        CHECK(calls == 1);
    }
    SECTION("a listener may remove itself while the event is raised") {
        gles::GlesTexture2D tex(describe(2, 2, pixels));
        tex.initialize();
        int first = 0, second = 0;
        uint32_t firstId = 0;
        firstId = tex.addOnDisposedListener([&](ris::ATexture2D& t) {
            ++first;
            t.removeOnDisposedListener(firstId);
        });
        tex.addOnDisposedListener([&](ris::ATexture2D&) { ++second; });
        REQUIRE_NOTHROW(tex.dispose());
        CHECK(first == 1);
        CHECK(second == 1);
    }
    SECTION("disposing a texture that was never initialized raises the event too") {
        int calls = 0;
        gles::GlesTexture2D tex(describe(2, 2, pixels));
        tex.addOnDisposedListener([&](ris::ATexture2D&) { ++calls; });
        tex.dispose();
        CHECK(tex.getTextureState() == ris::State::Disposed);
        CHECK(calls == 1);
    }
}

TEST_CASE("GlesTexture2D default texture of a framework") {
    if (!ris_test::display_available()) SKIP("no display available");
    int disposedByFramework = 0;
    bool listenerAdded = false;
    {
        FrameworkConfiguration config = ris_createFrameworkConfiguration();
        ris::Framework framework(config);

        gles::GlesTexture2D* first = gles::GlesTexture2D::getOrCreateDefault(framework);
        REQUIRE(first != nullptr);
        CHECK(first->getTextureState() == ris::State::Initialized);
        CHECK(first->getWidth() == 1);
        CHECK(first->getHeight() == 1);
        CHECK(first->getSize() == 4);
        CHECK(first->getLabel() == "RisDefaultFilledTexture");
        CHECK(first->getTextureFormat() == ris::TextureFormat::Rgba8Unorm);
        CHECK(first->getGlTexture() != 0);

        SECTION("is reused") {
            CHECK(gles::GlesTexture2D::getOrCreateDefault(framework) == first);
        }
        SECTION("is created again after it was disposed") {
            first->dispose();
            gles::GlesTexture2D* second = gles::GlesTexture2D::getOrCreateDefault(framework);
            CHECK(second->getTextureState() == ris::State::Initialized);
            CHECK(second->getGlTexture() != 0);
        }
        SECTION("disposeDefault disposes and forgets it, a later call makes a new one") {
            int calls = 0;
            first->addOnDisposedListener([&](ris::ATexture2D&) { ++calls; });
            gles::GlesTexture2D::disposeDefault(framework);
            CHECK(calls == 1);
            CHECK_NOTHROW(gles::GlesTexture2D::disposeDefault(framework));   // nothing left
            gles::GlesTexture2D* again = gles::GlesTexture2D::getOrCreateDefault(framework);
            CHECK(again->getTextureState() == ris::State::Initialized);
        }
        SECTION("the framework disposes it when it is destroyed") {
            gles::GlesTexture2D::getOrCreateDefault(framework)->addOnDisposedListener(
                [&](ris::ATexture2D&) { ++disposedByFramework; });
            listenerAdded = true;
        }
    }
    CHECK(disposedByFramework == (listenerAdded ? 1 : 0));
}
