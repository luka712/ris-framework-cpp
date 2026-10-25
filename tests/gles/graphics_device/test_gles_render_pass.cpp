// Tests of GlesRenderPass and GlesSwapChain created by the graphics device (need a display).
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <stdexcept>

#include <ris/ris.hpp>

#include "../gles_test_utils.hpp"

namespace gles = ris::gles;
namespace fu = ris::gles::framebuffer_utilities;

namespace {

struct Fixture {
    ris::Window window = ris_test::make_hidden_window();
    gles::GlesGraphicsDevice device;
    Fixture() { device.initialize(); }
};

/// Reads the pixel (x, y) of the texture through a temporary framebuffer.
ris_test::Pixel texel(const ris::Texture& texture, int x, int y) {
    gles::GlUint fb = fu::create(texture.gl_handle());
    fu::bind(fb);
    const ris_test::Pixel px = ris_test::read_pixel(x, y);
    fu::bind(0);
    fu::delete_framebuffer(fb);
    return px;
}

ris::RenderPassDescriptor textureDescriptor(ris::Texture& texture, ris::Color clear) {
    ris::RenderPassColorAttachment attachment;
    attachment.texture = &texture;
    attachment.clearColor = clear;
    ris::RenderPassDescriptor descriptor;
    descriptor.colorAttachments.push_back(attachment);
    descriptor.label = "test pass";
    return descriptor;
}

} // namespace

TEST_CASE("render pass clears a texture") {
    if (!ris_test::display_available()) SKIP("no display available");
    Fixture f;
    ris::Texture texture = ris::Texture::from_rgba8(8, 4, nullptr, false);

    SECTION("clear to a colour and read a pixel back") {
        const std::unique_ptr<ris::ARenderPass> pass(f.device.createRenderPass(textureDescriptor(texture, ris::Color(1, 0, 0, 1))));
        REQUIRE(pass != nullptr);
        pass->beginPass();
        pass->endPass();
        CHECK(texel(texture, 0, 0) == ris_test::Pixel{255, 0, 0, 255});
        CHECK(texel(texture, 7, 3) == ris_test::Pixel{255, 0, 0, 255});

        // The pass is reusable, a second pass with another colour overwrites the first.
        const std::unique_ptr<ris::ARenderPass> second(f.device.createRenderPass(textureDescriptor(texture, ris::Color(0, 0, 1, 1))));
        second->beginPass();
        second->endPass();
        CHECK(texel(texture, 3, 2) == ris_test::Pixel{0, 0, 255, 255});
    }

    SECTION("drawing happens between begin and end, load keeps the contents") {
        auto descriptor = textureDescriptor(texture, ris::Color(0, 1, 0, 1));
        const std::unique_ptr<ris::ARenderPass> clear(f.device.createRenderPass(descriptor));
        clear->beginPass();
        clear->endPass();

        descriptor.colorAttachments[0].loadAction = ris::LoadAction::Load;
        descriptor.colorAttachments[0].clearColor = ris::Color(1, 0, 0, 1); // ignored
        const std::unique_ptr<ris::ARenderPass> load(f.device.createRenderPass(descriptor));
        load->beginPass();
        load->endPass();
        CHECK(texel(texture, 1, 1) == ris_test::Pixel{0, 255, 0, 255});
    }

    SECTION("a depth and stencil attachment turns the depth test on and clears the depth") {
        auto descriptor = textureDescriptor(texture, ris::Color(0, 0, 0, 1));
        descriptor.depthStencilAttachment = ris::RenderPassDepthStencilAttachment{};
        ris_test::FullscreenTriangle triangle; // at depth 0, which is 0.5 in window coordinates

        // Cleared to 1 (the far plane): the triangle is in front and is drawn.
        const std::unique_ptr<ris::ARenderPass> far(f.device.createRenderPass(descriptor));
        far->beginPass();
        triangle.draw(0, 1, 1, 1);
        far->endPass();
        CHECK(texel(texture, 2, 1) == ris_test::Pixel{0, 255, 255, 255});

        // Cleared to 0 (the near plane): the triangle is behind and fails the depth test.
        descriptor.depthStencilAttachment->depthClearValue = 0.0f;
        const std::unique_ptr<ris::ARenderPass> near(f.device.createRenderPass(descriptor));
        near->beginPass();
        triangle.draw(0, 1, 1, 1);
        near->endPass();
        CHECK(texel(texture, 2, 1) == ris_test::Pixel{0, 0, 0, 255});
    }

    SECTION("discard store actions are accepted") {
        auto descriptor = textureDescriptor(texture, ris::Color(1, 1, 1, 1));
        descriptor.colorAttachments[0].storeAction = ris::StoreAction::Discard;
        descriptor.depthStencilAttachment = ris::RenderPassDepthStencilAttachment{};
        const std::unique_ptr<ris::ARenderPass> pass(f.device.createRenderPass(descriptor));
        pass->beginPass();
        CHECK_NOTHROW(pass->endPass());
    }

    SECTION("begin and end must be paired") {
        const std::unique_ptr<ris::ARenderPass> pass(f.device.createRenderPass(textureDescriptor(texture, ris::Color::white())));
        CHECK_THROWS_AS(pass->endPass(), std::invalid_argument);
        pass->beginPass();
        CHECK_THROWS_AS(pass->beginPass(), std::invalid_argument);
        pass->endPass();
        CHECK_THROWS_AS(pass->endPass(), std::invalid_argument);
    }
}

TEST_CASE("render pass with several colour attachments") {
    if (!ris_test::display_available()) SKIP("no display available");
    Fixture f;
    ris::Texture a = ris::Texture::from_rgba8(4, 4, nullptr, false);
    ris::Texture b = ris::Texture::from_rgba8(4, 4, nullptr, false);
    ris::RenderPassDescriptor descriptor = textureDescriptor(a, ris::Color(1, 0, 0, 1));
    descriptor.colorAttachments.push_back({});
    descriptor.colorAttachments[1].texture = &b;
    descriptor.colorAttachments[1].clearColor = ris::Color(0, 1, 0, 1);

    const std::unique_ptr<ris::ARenderPass> pass(f.device.createRenderPass(descriptor));
    pass->beginPass();
    pass->endPass();
    CHECK(texel(a, 1, 1) == ris_test::Pixel{255, 0, 0, 255});
    CHECK(texel(b, 1, 1) == ris_test::Pixel{0, 255, 0, 255});
}

TEST_CASE("invalid render pass descriptors throw std::invalid_argument") {
    if (!ris_test::display_available()) SKIP("no display available");
    Fixture f;
    ris::Texture texture = ris::Texture::from_rgba8(4, 4, nullptr, false);
    const std::unique_ptr<ris::ASwapChain> swapChain(f.device.createSwapChain(f.window, {}));

    SECTION("no attachment") {
        CHECK_THROWS_AS(f.device.createRenderPass({}), std::invalid_argument);
    }
    SECTION("neither a swap chain nor a texture") {
        ris::RenderPassDescriptor descriptor;
        descriptor.colorAttachments.push_back({});
        CHECK_THROWS_AS(f.device.createRenderPass(descriptor), std::invalid_argument);
    }
    SECTION("both a swap chain and a texture") {
        ris::RenderPassDescriptor descriptor = textureDescriptor(texture, ris::Color::black());
        descriptor.colorAttachments[0].swapChain = swapChain.get();
        CHECK_THROWS_AS(f.device.createRenderPass(descriptor), std::invalid_argument);
    }
    SECTION("a swap chain together with another attachment") {
        ris::RenderPassDescriptor descriptor = textureDescriptor(texture, ris::Color::black());
        ris::RenderPassColorAttachment screen;
        screen.swapChain = swapChain.get();
        descriptor.colorAttachments.push_back(screen);
        CHECK_THROWS_AS(f.device.createRenderPass(descriptor), std::invalid_argument);
    }
}

TEST_CASE("swap chain") {
    if (!ris_test::display_available()) SKIP("no display available");
    Fixture f;

    SECTION("size, format and present") {
        const std::unique_ptr<ris::ASwapChain> swapChain(f.device.createSwapChain(f.window, {}));
        REQUIRE(swapChain != nullptr);
        const ris::Vec2 size = swapChain->getBackBufferSize();
        CHECK(size.x == 64.0f);
        CHECK(size.y == 64.0f);
        CHECK(swapChain->getTextureFormat() == ris::TextureFormat::Rgba8Unorm);
        CHECK_NOTHROW(swapChain->present());
        CHECK_NOTHROW(swapChain->resize(32, 32));
        CHECK_THROWS_AS(swapChain->resize(0, 32), std::invalid_argument);
        CHECK_THROWS_AS(swapChain->resize(32, -1), std::invalid_argument);
    }

    SECTION("the present mode sets the vsync of the window") {
        ris::SwapChainDescriptor descriptor;
        descriptor.desiredPresentationMode = ris::SurfacePresentMode::Immediate;
        const std::unique_ptr<ris::ASwapChain> immediate(f.device.createSwapChain(f.window, descriptor));
        CHECK(static_cast<gles::GlesSwapChain*>(immediate.get())->getPresentMode() == ris::SurfacePresentMode::Immediate);
        descriptor.desiredPresentationMode = ris::SurfacePresentMode::Mailbox;
        CHECK_NOTHROW(f.device.createSwapChain(f.window, descriptor)->present());
    }

    SECTION("invalid descriptors") {
        ris::SwapChainDescriptor descriptor;
        descriptor.width = -1;
        CHECK_THROWS_AS(f.device.createSwapChain(f.window, descriptor), std::invalid_argument);
        descriptor = {};
        descriptor.desiredPresentationMode = static_cast<ris::SurfacePresentMode>(77);
        CHECK_THROWS_AS(f.device.createSwapChain(f.window, descriptor), std::domain_error);
    }

    SECTION("a render pass draws to the window and clears it") {
        const std::unique_ptr<ris::ASwapChain> swapChain(f.device.createSwapChain(f.window, {}));
        ris::RenderPassColorAttachment screen;
        screen.swapChain = swapChain.get();
        screen.clearColor = ris::Color(0, 0, 1, 1);
        ris::RenderPassDescriptor descriptor;
        descriptor.colorAttachments.push_back(screen);
        descriptor.depthStencilAttachment = ris::RenderPassDepthStencilAttachment{};
        const std::unique_ptr<ris::ARenderPass> pass(f.device.createRenderPass(descriptor));
        pass->beginPass();
        CHECK(ris_test::read_pixel(10, 10) == ris_test::Pixel{0, 0, 255, 255});
        pass->endPass();
        swapChain->present();
    }
}
