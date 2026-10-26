// Tests of ATexture2D / GlesTexture2D / GlesTextureView (need a display).
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include <ris/ris.hpp>

#if __has_include(<ris_ktx2/ris_ktx.hpp>)
#include <ris_ktx2/ris_ktx.hpp>
#else
#include <ris_ktx.hpp>
#endif

#include "../gles_test_utils.hpp"

namespace gles = ris::gles;

namespace {

std::vector<uint8_t> rgba_pixels(int width, int height) {
    return std::vector<uint8_t>(static_cast<size_t>(width) * static_cast<size_t>(height) * 4, 200);
}

} // namespace

TEST_CASE("ATexture2D properties after creation") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    const gles::GlesTextureFactory factory;
    const auto pixels = rgba_pixels(8, 4);

    const std::unique_ptr<ris::ATexture2D> tex(
        factory.create(8, 4, pixels, 4, "properties", ris::TextureUsage::TextureBinding, ris::TextureFormat::Rgba8Unorm, false));
    REQUIRE(tex != nullptr);
    CHECK(tex->getWidth() == 8);
    CHECK(tex->getHeight() == 4);
    CHECK(tex->getMipLevels() == 1);
    CHECK(tex->getTextureUsage() == ris::TextureUsage::TextureBinding);
    CHECK(tex->getTextureFormat() == ris::TextureFormat::Rgba8Unorm);
    CHECK(tex->getTextureViewFormat() == ris::TextureFormat::Rgba8Unorm);
    CHECK(tex->getHandle() != 0);
    CHECK(tex->getId() != 0);
    CHECK(tex->getLabel() == "properties");
    CHECK(tex->getTextureState() == ris::State::Initialized);
    CHECK(tex->getSize() == 8 * 4 * 4);

    SECTION("the GL ES texture exposes the same handle") {
        const auto* gl = dynamic_cast<const gles::GlesTexture2D*>(tex.get());
        REQUIRE(gl != nullptr);
        CHECK(gl->getGlTexture() == tex->getHandle());
        CHECK(gl->getWidth() == tex->getWidth());
        CHECK(gl->getHeight() == tex->getHeight());
        CHECK(gl->getMipLevels() == tex->getMipLevels());
    }
    SECTION("sRGB data gets the sRGB format") {
        const std::unique_ptr<ris::ATexture2D> srgb(factory.create(8, 4, pixels, 4, {}, ris::TextureUsage::TextureBinding,
                                                                    ris::TextureFormat::Rgba8UnormSrgb, false));
        CHECK(srgb->getTextureFormat() == ris::TextureFormat::Rgba8UnormSrgb);
        CHECK(srgb->getTextureViewFormat() == ris::TextureFormat::Rgba8UnormSrgb);
        CHECK(srgb->getLabel().empty());
    }
}

TEST_CASE("ATexture2D mip levels") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    const gles::GlesTextureFactory factory;
    const auto pixels = rgba_pixels(4, 4);

    SECTION("without mip maps there is one level") {
        const std::unique_ptr<ris::ATexture2D> tex(
            factory.create(4, 4, pixels, 4, {}, ris::TextureUsage::TextureBinding, ris::TextureFormat::Rgba8Unorm, false));
        CHECK(tex->getMipLevels() == 1);
        CHECK(tex->getSize() == 4 * 4 * 4);
    }
    SECTION("generateMipmaps gives the whole chain and the size counts every level") {
        const std::unique_ptr<ris::ATexture2D> tex(
            factory.create(4, 4, pixels, 4, {}, ris::TextureUsage::TextureBinding, ris::TextureFormat::Rgba8Unorm, true));
        CHECK(tex->getMipLevels() == 3);
        CHECK(tex->getSize() == (16 + 4 + 1) * 4);
    }
    SECTION("a non square texture ends with a 1x1 level") {
        const auto big = rgba_pixels(8, 2);
        const std::unique_ptr<ris::ATexture2D> tex(
            factory.create(8, 2, big, 4, {}, ris::TextureUsage::TextureBinding, ris::TextureFormat::Rgba8Unorm, true));
        CHECK(tex->getMipLevels() == 4);
        CHECK(tex->getSize() == (16 + 4 + 2 + 1) * 4);
    }
    SECTION("ris::gles::GlesTexture2D reports the same values") {
        ris::gles::GlesTexture2D t = ris::gles::GlesTexture2D::from_rgba8(4, 4, pixels.data(), true, false, ris::TextureUsage::TextureBinding, "legacy");
        CHECK(t.getMipLevels() == 3);
        CHECK(t.getLabel() == "legacy");
        CHECK(t.getTextureUsage() == ris::TextureUsage::TextureBinding);
    }
}

TEST_CASE("ATexture2D createEmpty size") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    const gles::GlesTextureFactory factory;
    const std::unique_ptr<ris::ATexture2D> tex(factory.createEmpty(16, 8, std::nullopt, ris::TextureUsage::CopyDst, ris::TextureFormat::Rgba8Unorm, "empty"));
    CHECK(tex->getSize() == 16 * 8 * 4);
    CHECK(tex->getLabel() == "empty");
    CHECK(tex->getTextureUsage() == ris::TextureUsage::CopyDst);
}

TEST_CASE("ATexture2D state transitions") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    const gles::GlesTextureFactory factory;

    SECTION("factory textures are initialized, a second initialize() throws") {
        const std::unique_ptr<ris::ATexture2D> tex(factory.createEmpty(2, 2));
        CHECK(tex->getTextureState() == ris::State::Initialized);
        CHECK_THROWS_AS(tex->initialize(), std::logic_error);
        CHECK(tex->getTextureState() == ris::State::Initialized);
    }
    SECTION("a moved-from texture is disposed and empty") {
        gles::GlesTexture2D a = ris::gles::GlesTexture2D::from_rgba8(2, 2, nullptr, false);
        const uint64_t handle = a.getHandle();
        const uint32_t id = a.getId();
        gles::GlesTexture2D b(std::move(a));
        CHECK(b.getTextureState() == ris::State::Initialized);
        CHECK(b.getHandle() == handle);
        CHECK(b.getId() == id);
        CHECK(a.getTextureState() == ris::State::Disposed);  // NOLINT(bugprone-use-after-move): the state is defined
        CHECK(a.getHandle() == 0);
        CHECK_THROWS_AS(a.initialize(), std::logic_error);
        CHECK_THROWS_AS(a.createView(), std::logic_error);
        CHECK_THROWS_AS(a.getDefaultTextureView(), std::logic_error);
    }
    SECTION("move assignment disposes the old texture and moves the default view") {
        gles::GlesTexture2D a = ris::gles::GlesTexture2D::from_rgba8(2, 2, nullptr, false);
        gles::GlesTexture2D b = ris::gles::GlesTexture2D::from_rgba8(4, 4, nullptr, false);
        int disposed = 0;
        a.addOnDisposedListener([&](ris::ATexture2D& t) {
            CHECK(t.getWidth() == 2);
            ++disposed;
        });
        a = std::move(b);
        CHECK(disposed == 1);
        CHECK(a.getWidth() == 4);
        CHECK(a.getTextureState() == ris::State::Initialized);
        const auto& view = dynamic_cast<const gles::GlesTextureView&>(a.getDefaultTextureView());
        CHECK(&view.getTexture() == &a);   // the view points at the new owner
    }
}

TEST_CASE("ATexture2D default texture view") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    const gles::GlesTextureFactory factory;
    const std::unique_ptr<ris::ATexture2D> tex(factory.createEmpty(4, 4, std::nullopt, ris::TextureUsage::TextureBinding, ris::TextureFormat::Rgba8Unorm, "tex"));

    const ris::ATextureView& view = tex->getDefaultTextureView();
    CHECK(&view == &tex->getDefaultTextureView());   // owned by the texture: always the same object
    CHECK(view.getTextureFormat() == ris::TextureFormat::Rgba8Unorm);
    CHECK(view.getDimension() == ris::TextureViewDimension::Dimension2D);
    const auto& glView = dynamic_cast<const gles::GlesTextureView&>(view);
    CHECK(&glView.getTexture() == tex.get());
    CHECK_NOTHROW(glView.bind(0));
}

TEST_CASE("ATexture2D createView") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    const gles::GlesTextureFactory factory;
    const std::unique_ptr<ris::ATexture2D> tex(factory.createEmpty(4, 4));

    SECTION("the default descriptor gives a 2D view of the texture view format") {
        const std::unique_ptr<ris::ATextureView> view(tex->createView());
        REQUIRE(view != nullptr);
        CHECK(view.get() != &tex->getDefaultTextureView());   // owned by the caller
        CHECK(view->getDimension() == ris::TextureViewDimension::Dimension2D);
        CHECK(view->getTextureFormat() == ris::TextureFormat::Rgba8Unorm);
    }
    SECTION("label and an explicit equal format") {
        ris::TextureViewDescriptor descriptor;
        descriptor.label = "my view";
        descriptor.textureFormat = ris::TextureFormat::Rgba8Unorm;
        const std::unique_ptr<gles::GlesTextureView> view(static_cast<gles::GlesTexture2D&>(*tex).createView(descriptor));
        CHECK(view->getLabel() == "my view");
        CHECK(&view->getTexture() == tex.get());
    }
    SECTION("views can be created many times") {
        const std::unique_ptr<ris::ATextureView> a(tex->createView());
        const std::unique_ptr<ris::ATextureView> b(tex->createView());
        CHECK(a != b);
    }
    SECTION("a dimension a 2D texture cannot have is an invalid argument") {
        for (const auto dimension : {ris::TextureViewDimension::Undefined, ris::TextureViewDimension::Dimension1D,
                                     ris::TextureViewDimension::Dimension2DArray, ris::TextureViewDimension::DimensionCube,
                                     ris::TextureViewDimension::DimensionCubeArray, ris::TextureViewDimension::Dimension3D}) {
            ris::TextureViewDescriptor descriptor;
            descriptor.dimension = dimension;
            CHECK_THROWS_AS(tex->createView(descriptor), std::invalid_argument);
        }
    }
    SECTION("another format is not supported") {
        ris::TextureViewDescriptor descriptor;
        descriptor.textureFormat = ris::TextureFormat::Rgba8UnormSrgb;
        CHECK_THROWS_AS(tex->createView(descriptor), std::domain_error);
    }
}

TEST_CASE("ATexture2D disposed listeners") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    const gles::GlesTextureFactory factory;

    SECTION("the listener fires once when the texture is destroyed") {
        int calls = 0;
        ris::State state = ris::State::Created;
        uint64_t handle = 1;
        uint32_t id = 0;
        uint32_t seenId = 0;
        {
            std::unique_ptr<ris::ATexture2D> tex(factory.createEmpty(2, 2));
            id = tex->getId();
            tex->addOnDisposedListener([&](ris::ATexture2D& t) {
                ++calls;
                state = t.getTextureState();
                handle = t.getHandle();
                seenId = t.getId();
            });
            CHECK(calls == 0);
        }
        CHECK(calls == 1);
        CHECK(state == ris::State::Disposed);
        CHECK(handle == 0);
        CHECK(seenId == id);
    }
    SECTION("a removed listener does not fire, the others do") {
        int a = 0, b = 0;
        {
            std::unique_ptr<ris::ATexture2D> tex(factory.createEmpty(2, 2));
            const uint32_t idA = tex->addOnDisposedListener([&](ris::ATexture2D&) { ++a; });
            const uint32_t idB = tex->addOnDisposedListener([&](ris::ATexture2D&) { ++b; });
            CHECK(idB > idA);
            CHECK(tex->removeOnDisposedListener(idA));
            CHECK_FALSE(tex->removeOnDisposedListener(idA));   // already removed
            CHECK_FALSE(tex->removeOnDisposedListener(0));     // never existed
        }
        CHECK(a == 0);
        CHECK(b == 1);
    }
    SECTION("listener ids are unique and increasing") {
        const std::unique_ptr<ris::ATexture2D> t1(factory.createEmpty(2, 2));
        const std::unique_ptr<ris::ATexture2D> t2(factory.createEmpty(2, 2));
        const uint32_t i1 = t1->addOnDisposedListener([](ris::ATexture2D&) {});
        const uint32_t i2 = t2->addOnDisposedListener([](ris::ATexture2D&) {});
        const uint32_t i3 = t1->addOnDisposedListener([](ris::ATexture2D&) {});
        CHECK(i1 >= 1);
        CHECK(i2 > i1);
        CHECK(i3 > i2);
    }
    SECTION("an empty listener is rejected") {
        const std::unique_ptr<ris::ATexture2D> tex(factory.createEmpty(2, 2));
        CHECK_THROWS_AS(tex->addOnDisposedListener({}), std::invalid_argument);
    }
    SECTION("a listener that throws does not stop the destruction") {
        int later = 0;
        {
            std::unique_ptr<ris::ATexture2D> tex(factory.createEmpty(2, 2));
            tex->addOnDisposedListener([](ris::ATexture2D&) { throw std::runtime_error("listener"); });
            tex->addOnDisposedListener([&](ris::ATexture2D&) { ++later; });
        }
        CHECK(later == 1);
    }
    SECTION("a moved texture raises the event once, for the new owner") {
        int calls = 0;
        {
            gles::GlesTexture2D a = ris::gles::GlesTexture2D::from_rgba8(2, 2, nullptr, false);
            a.addOnDisposedListener([&](ris::ATexture2D&) { ++calls; });
            gles::GlesTexture2D b(std::move(a));
            CHECK(calls == 0);
        }
        CHECK(calls == 1);
    }
}

TEST_CASE("ATexture2D ids are unique and increasing") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    const gles::GlesTextureFactory factory;
    const std::unique_ptr<ris::ATexture2D> a(factory.createEmpty(2, 2));
    const std::unique_ptr<ris::ATexture2D> b(factory.createEmpty(2, 2));
    const gles::GlesTexture2D c = ris::gles::GlesTexture2D::from_rgba8(2, 2, nullptr, false);
    CHECK(a->getId() >= 1);
    CHECK(b->getId() > a->getId());
    CHECK(c.getId() > b->getId());
}

TEST_CASE("ATexture2D from a KTX2 file") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    const gles::GlesTextureFactory factory;
    ktxTexture2* ktx = nullptr;
    REQUIRE(ris_ktxTexture2_CreateFromNamedFile(RIS_TEST_DATA_DIR "/test_basis_uastc.ktx2", KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx) == KTX_SUCCESS);
    const uint32_t levels = ktx->numLevels;
    const int32_t width = static_cast<int32_t>(ktx->baseWidth), height = static_cast<int32_t>(ktx->baseHeight);

    SECTION("an RGBA8 target reports the format, mips and size") {
        ris::TextureDescriptor descriptor;
        descriptor.textureFormat = ris::TextureFormat::Rgba8Unorm;
        descriptor.textureUsage = ris::TextureUsage::TextureBinding;
        descriptor.label = "ktx";
        const std::unique_ptr<ris::ATexture2D> tex(factory.createFromKtx2(ktx, descriptor));
        CHECK(tex->getTextureFormat() == ris::TextureFormat::Rgba8Unorm);
        CHECK(tex->getTextureViewFormat() == ris::TextureFormat::Rgba8Unorm);
        CHECK(tex->getTextureUsage() == ris::TextureUsage::TextureBinding);
        CHECK(tex->getLabel() == "ktx");
        CHECK(tex->getWidth() == width);
        CHECK(tex->getHeight() == height);
        CHECK(tex->getMipLevels() == levels);   // the levels of the file, generateMipmaps is off
        CHECK(tex->getSize() >= static_cast<uint64_t>(width) * static_cast<uint64_t>(height) * 4);
        CHECK(tex->getTextureState() == ris::State::Initialized);
        CHECK(tex->getHandle() != 0);
    }
    SECTION("generateMipmaps of a single level file gives the whole chain") {
        ris::TextureDescriptor descriptor;
        descriptor.textureFormat = ris::TextureFormat::Rgba8Unorm;
        descriptor.generateMipmaps = true;
        const std::unique_ptr<ris::ATexture2D> tex(factory.createFromKtx2(ktx, descriptor));
        CHECK(tex->getMipLevels() == (levels == 1 ? GlesUtilities::texture().fullMipCount(width, height) : levels));
    }
    SECTION("the default target is a format the GPU supports") {
        const std::unique_ptr<ris::ATexture2D> tex(factory.createFromKtx2(ktx));
        const ris::TextureFormat format = tex->getTextureFormat();
        CHECK((format == ris::TextureFormat::Rgba8Unorm || format == ris::TextureFormat::Astc4x4Rgba ||
               format == ris::TextureFormat::Etc2Rgba8Unorm || format == ris::TextureFormat::Bc3RgbaUnorm ||
               format == ris::TextureFormat::Bc7RgbaUnorm));
        CHECK(tex->getMipLevels() == levels);
        CHECK(tex->getSize() > 0);
    }
    ris_ktxTexture2_Destroy(ktx);
}
