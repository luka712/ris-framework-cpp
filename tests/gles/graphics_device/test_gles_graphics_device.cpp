// Tests of GlesGraphicsDevice / AGraphicsDevice (need a display): GPU info, features, defaults, debug groups.
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <memory>
#include <stdexcept>

#include <ris/ris.hpp>

#include "../gles_test_utils.hpp"

namespace gles = ris::gles;

TEST_CASE("the default graphics device is the OpenGL ES one") {
    const std::unique_ptr<ris::AGraphicsDevice> device = ris::create_default_graphics_device();
    REQUIRE(dynamic_cast<gles::GlesGraphicsDevice*>(device.get()) != nullptr);
}

TEST_CASE("an uninitialised graphics device throws std::logic_error") {
    gles::GlesGraphicsDevice device;
    CHECK_THROWS_AS(device.getGpuInfo(), std::logic_error);
    CHECK_THROWS_AS(device.getFeatures(), std::logic_error);
    CHECK_THROWS_AS(device.getDefaultTextureSampler(), std::logic_error);
    CHECK_THROWS_AS(device.getDefaultBlendState(), std::logic_error);
    CHECK_THROWS_AS(device.getDefaultPrimitiveState(), std::logic_error);
    CHECK_THROWS_AS(device.setupDebugCallback(), std::logic_error);
    CHECK_THROWS_AS(device.frameEnd(), std::logic_error);
    CHECK_THROWS_AS(device.createSampler({}), std::logic_error);
    CHECK_THROWS_AS(device.createPrimitiveState({}), std::logic_error);
    CHECK_THROWS_AS(device.pushDebugGroup("group"), std::logic_error);
    CHECK_THROWS_AS(device.popDebugGroup(), std::logic_error);
    // No window exists here either: initialize() reports the missing context.
    CHECK_THROWS_AS(device.initialize(), std::logic_error);
}

TEST_CASE("graphics device initialize, GPU info and features") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    gles::GlesGraphicsDevice device;
    device.initialize();

    SECTION("initialize twice is an error") {
        CHECK_THROWS_AS(device.initialize(), std::invalid_argument);
    }

    SECTION("GPU info is read from the GL strings") {
        const ris::GpuInfo& info = device.getGpuInfo();
        CHECK_FALSE(info.name.empty());
        CHECK_FALSE(info.vendor.empty());
        CHECK_FALSE(info.apiVersion.empty());
        CHECK_FALSE(info.shadingLanguageVersion.empty());
        CHECK(info.api == ris::RenderingBackend::OpenGLES);
        CHECK(info.name == window.gl_info(ris::GlInfo::Renderer));
        CHECK(info.apiVersion == window.gl_info(ris::GlInfo::Version));
    }

    SECTION("features agree with the extensions and the compressed format list") {
        const ris::AGraphicsDeviceFeatures& features = device.getFeatures();
        CHECK(features.supportsTextureCompressionETC2()); // core in OpenGL ES 3
        CHECK(features.supportsTextureCompressionASTC() == window.has_gl_extension("GL_KHR_texture_compression_astc_ldr"));
        if (features.supportsTextureCompressionBC()) CHECK(features.supportsTextureCompressionS3TC());

        const auto& formats = device.getGpuInfo().supportedCompressedTextureFormats;
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
    ris::Window window = ris_test::make_hidden_window();

    ris::GraphicsDeviceDescriptor descriptor;
    descriptor.samplerFilteringPreset = ris::TextureFilteringPreset::Nearest;
    gles::GlesGraphicsDevice nearest(descriptor);
    nearest.initialize();
    CHECK(nearest.getDefaultTextureSampler().getMinFilter() == ris::SamplerFilter::Nearest);
    CHECK(nearest.getDefaultTextureSampler().getMagFilter() == ris::SamplerFilter::Nearest);
    CHECK(nearest.getDefaultTextureSampler().getMipmapFilter() == ris::MipMapSamplerFilter::None);

    descriptor.samplerFilteringPreset = ris::TextureFilteringPreset::Bilinear;
    gles::GlesGraphicsDevice bilinear(descriptor);
    bilinear.initialize();
    CHECK(bilinear.getDefaultTextureSampler().getMinFilter() == ris::SamplerFilter::Linear);
    CHECK(bilinear.getDefaultTextureSampler().getMipmapFilter() == ris::MipMapSamplerFilter::None);
}

TEST_CASE("graphics device debug functions are safe") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris::Window window = ris_test::make_hidden_window();
    gles::GlesGraphicsDevice device;
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

    SECTION("pop without push is an error") {
        CHECK_THROWS_AS(device.popDebugGroup(), std::invalid_argument);
    }

    SECTION("frameEnd is a no-op") {
        CHECK_NOTHROW(device.frameEnd());
    }
}
