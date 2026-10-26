// Tests of GlesGraphicsDevice / AGraphicsDevice (need a display): GPU info, features, defaults, debug groups.
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>

#include <ris/ris.hpp>

#include "../gles_test_utils.hpp"

namespace gles = ris::gles;

TEST_CASE("the default graphics device is the OpenGL ES one") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;
    const std::unique_ptr<ris::AGraphicsDevice> device = ris::create_default_graphics_device(window);
    REQUIRE(dynamic_cast<gles::GlesGraphicsDevice*>(device.get()) != nullptr);
}

TEST_CASE("an uninitialised graphics device throws std::logic_error") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;
    gles::GlesGraphicsDevice device(window);
    CHECK_THROWS_AS(device.getGpuInfo(), std::logic_error);
    CHECK_THROWS_AS(device.getFeatures(), std::logic_error);
    CHECK_THROWS_AS(device.getDefaultTextureSampler(), std::logic_error);
    CHECK_THROWS_AS(device.getDefaultBlendState(), std::logic_error);
    CHECK_THROWS_AS(device.getDefaultPrimitiveState(), std::logic_error);
    CHECK_THROWS_AS(device.setupDebugCallback(), std::logic_error);
    CHECK_THROWS_AS(device.frameEnd(), std::logic_error);
    CHECK_THROWS_AS(device.createSampler({}), std::logic_error);
    CHECK_THROWS_AS(device.createPrimitiveState({}), std::logic_error);
}

TEST_CASE("graphics device initialize, GPU info and features") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;
    gles::GlesGraphicsDevice device(window);
    device.initialize();

    SECTION("initialize twice is an error") {
        CHECK_THROWS_AS(device.initialize(), std::invalid_argument);
    }

    SECTION("GPU info is read from the GL strings") {
        const ris::AGpuInfo& info = device.getGpuInfo();
        CHECK(dynamic_cast<const gles::GlesGpuInfo*>(&info) != nullptr);
        CHECK(&info == &device.getGpuInfo());   // owned by the device
        CHECK_FALSE(info.getName().empty());
        CHECK_FALSE(info.getVendor().empty());
        CHECK_FALSE(info.getApiVersion().empty());
        CHECK(info.getApi() == ris::RenderingBackend::OpenGLES);
        // OpenGL ES is detected from the GL strings, the API below it (ANGLE's) is not reported through GL.
        const bool es = std::string(GlesUtilities::context().glString(GL_VERSION)).find("OpenGL ES") != std::string::npos ||
                        std::string(GlesUtilities::context().glString(GL_RENDERER)).find("OpenGL ES") != std::string::npos;
        CHECK(info.getUnderlyingBackend() == (es ? ris::RenderingBackend::OpenGLES : ris::RenderingBackend::Default));
        CHECK(info.getName() == GlesUtilities::context().glString(GL_RENDERER));
        CHECK(info.getApiVersion() == GlesUtilities::context().glString(GL_VERSION));
    }

    SECTION("features agree with the extensions and the compressed format list") {
        const ris::AGraphicsDeviceFeatures& features = device.getFeatures();
        CHECK(features.supportsTextureCompressionETC2()); // core in OpenGL ES 3
        CHECK(features.supportsTextureCompressionASTC() == GlesUtilities::context().isExtensionSupported("GL_KHR_texture_compression_astc_ldr"));
        if (features.supportsTextureCompressionBC()) CHECK(features.supportsTextureCompressionS3TC());

        const auto& formats = device.getGpuInfo().getSupportedCompressedTextureFormats();
        const auto has = [&](ris::TextureFormat f) { return std::find(formats.begin(), formats.end(), f) != formats.end(); };
        CHECK(has(ris::TextureFormat::Etc2Rgba8Unorm) == features.supportsTextureCompressionETC2());
        CHECK(has(ris::TextureFormat::Astc4x4Rgba) == features.supportsTextureCompressionASTC());
        CHECK(has(ris::TextureFormat::Bc3RgbaUnorm) == features.supportsTextureCompressionS3TC());
        CHECK(has(ris::TextureFormat::Bc7RgbaUnorm) == features.supportsTextureCompressionBC());
    }

    SECTION("defaults exist and are the documented ones") {
        const ris::ASampler& sampler = device.getDefaultTextureSampler();
        CHECK(sampler.getId() > 0);
        CHECK(sampler.getMinFilter() == ris::SamplerFilter::Linear);
        CHECK(sampler.getMipmapFilter() == ris::MipMapSamplerFilter::Linear); // Trilinear is the default preset
        CHECK(sampler.getAddressModeU() == ris::SamplerAddressMode::Repeat);

        CHECK(device.getDefaultBlendState().getDescriptor() == ris::BlendStateDescriptor{});
        const ris::APrimitiveState& primitive = device.getDefaultPrimitiveState();
        CHECK(primitive.getTopology() == ris::PrimitiveTopology::TriangleList);
        CHECK(primitive.getCullFace() == ris::CullMode::Back);
        CHECK(primitive.getFrontFace() == ris::FrontFace::Ccw);
    }
}

TEST_CASE("the default sampler follows the filtering preset") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;

    ris::GraphicsDeviceDescriptor descriptor;
    descriptor.samplerFilteringPreset = RIS_TEXTURE_FILTERING_NEAREST;
    gles::GlesGraphicsDevice nearest(window, descriptor);
    nearest.initialize();
    CHECK(nearest.getDefaultTextureSampler().getMinFilter() == ris::SamplerFilter::Nearest);
    CHECK(nearest.getDefaultTextureSampler().getMagFilter() == ris::SamplerFilter::Nearest);
    CHECK(nearest.getDefaultTextureSampler().getMipmapFilter() == ris::MipMapSamplerFilter::None);

    descriptor.samplerFilteringPreset = RIS_TEXTURE_FILTERING_BILINEAR;
    gles::GlesGraphicsDevice bilinear(window, descriptor);
    bilinear.initialize();
    CHECK(bilinear.getDefaultTextureSampler().getMinFilter() == ris::SamplerFilter::Linear);
    CHECK(bilinear.getDefaultTextureSampler().getMipmapFilter() == ris::MipMapSamplerFilter::None);
}

TEST_CASE("graphics device debug functions are safe") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;
    gles::GlesGraphicsDevice device(window);
    device.initialize();

    SECTION("setupDebugCallback can be called more than once") {
        CHECK_NOTHROW(device.setupDebugCallback());
        CHECK_NOTHROW(device.setupDebugCallback());
    }

    SECTION("debug groups nest") {
        CHECK_NOTHROW(device.pushDebugGroup("outer"));
        CHECK_NOTHROW(device.pushDebugGroup("inner"));
        CHECK_NOTHROW(device.popDebugGroup());
        CHECK_NOTHROW(device.popDebugGroup());
    }

    SECTION("push and pop do not throw") {
        CHECK_NOTHROW(device.pushDebugGroup("group"));
        CHECK_NOTHROW(device.popDebugGroup());
    }

    SECTION("frameEnd is a no-op") {
        CHECK_NOTHROW(device.frameEnd());
    }
}
