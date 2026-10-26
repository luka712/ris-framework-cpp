// Tests of AGpuInfo / GlesGpuInfo (need a display, except for the one without a context).
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include <ris/ris.hpp>

#include "../gles_test_utils.hpp"

namespace gles = ris::gles;

namespace {

/// Features with every flag set as the test wants it.
class FakeFeatures : public ris::AGraphicsDeviceFeatures {
public:
    FakeFeatures(bool s3tc, bool bc, bool etc2, bool astc) : s3tc_(s3tc), bc_(bc), etc2_(etc2), astc_(astc) {}
    bool supportsTextureCompressionS3TC() const override { return s3tc_; }
    bool supportsTextureCompressionBC() const override { return bc_; }
    bool supportsTextureCompressionETC2() const override { return etc2_; }
    bool supportsTextureCompressionASTC() const override { return astc_; }
    bool supportsTextureCompressionPVRTC() const override { return false; }

private:
    bool s3tc_, bc_, etc2_, astc_;
};

} // namespace

TEST_CASE("GlesGpuInfo without a window throws std::logic_error") {
    const FakeFeatures features(true, true, true, true);
    CHECK_THROWS_AS(gles::GlesGpuInfo(features), std::logic_error);
}

TEST_CASE("GlesGpuInfo reads the GL strings") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;
    const FakeFeatures features(false, false, true, false);
    const gles::GlesGpuInfo info(features);

    CHECK_FALSE(info.getName().empty());
    CHECK_FALSE(info.getVendor().empty());
    CHECK_FALSE(info.getApiVersion().empty());
    CHECK(info.getName() == GlesUtilities::context().glString(GL_RENDERER));
    CHECK(info.getVendor() == GlesUtilities::context().glString(GL_VENDOR));
    CHECK(info.getApiVersion() == GlesUtilities::context().glString(GL_VERSION));
    CHECK(info.getApi() == ris::RenderingBackend::OpenGLES);
    // OpenGL ES is detected from the GL strings, the API below it (ANGLE's) is not reported through GL.
    const bool es = std::string(GlesUtilities::context().glString(GL_VERSION)).find("OpenGL ES") != std::string::npos ||
                    std::string(GlesUtilities::context().glString(GL_RENDERER)).find("OpenGL ES") != std::string::npos;
    CHECK(info.getUnderlyingBackend() == (es ? ris::RenderingBackend::OpenGLES : ris::RenderingBackend::Default));
}

TEST_CASE("GlesGpuInfo lists the compressed formats of the features, in order") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;

    SECTION("everything") {
        const gles::GlesGpuInfo info(FakeFeatures(true, true, true, true));
        const std::vector<ris::TextureFormat> expected = {ris::TextureFormat::Astc4x4Rgba, ris::TextureFormat::Bc3RgbaUnorm,
                                                          ris::TextureFormat::Etc2Rgba8Unorm, ris::TextureFormat::Bc7RgbaUnorm};
        CHECK(info.getSupportedCompressedTextureFormats() == expected);
    }
    SECTION("nothing") {
        const gles::GlesGpuInfo info(FakeFeatures(false, false, false, false));
        CHECK(info.getSupportedCompressedTextureFormats().empty());
    }
    SECTION("only ETC2") {
        const gles::GlesGpuInfo info(FakeFeatures(false, false, true, false));
        CHECK(info.getSupportedCompressedTextureFormats() == std::vector<ris::TextureFormat>{ris::TextureFormat::Etc2Rgba8Unorm});
    }
}

TEST_CASE("the graphics device exposes its GPU info after initialize") {
    if (!ris_test::display_available()) SKIP("no display available");
    ris_test::HiddenWindow window;
    gles::GlesGraphicsDevice device(window);
    CHECK_THROWS_AS(device.getGpuInfo(), std::logic_error);
    device.initialize();
    const ris::AGpuInfo& info = device.getGpuInfo();
    CHECK_FALSE(info.getName().empty());
    CHECK(info.getApi() == ris::RenderingBackend::OpenGLES);
}
