// Needs a display (or Xvfb) and an EGL implementation (ANGLE, or Mesa software rendering on Linux).
#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <cstring>

#include <ris/ris.h>

namespace {
bool display_available() {
#if defined(__linux__)
    return std::getenv("DISPLAY") != nullptr || std::getenv("WAYLAND_DISPLAY") != nullptr;
#else
    return true;
#endif
}
} // namespace

TEST_CASE("window with a hidden window and a context") {
    if (!display_available()) {
        SKIP("no display available");
    }
    ris_window_desc desc;
    ris_window_desc_default(&desc);
    desc.backend = RIS_ANGLE_BACKEND_AUTO;
    desc.flags |= RIS_WINDOW_HIDDEN;
    desc.width = 64;
    desc.height = 64;

    ris_window* window = ris_window_create(&desc);
    REQUIRE(window != nullptr);

    SECTION("only one window at a time") {
        REQUIRE(ris_window_create(&desc) == nullptr);
    }
    SECTION("sizes") {
        int32_t w = 0, h = 0;
        ris_window_get_size(window, &w, &h);
        REQUIRE(w == 64);
        REQUIRE(h == 64);
    }
    SECTION("buffers on a real context") {
        ris_buffer* b = ris_buffer_create(RIS_BUFFER_VERTEX, RIS_USAGE_STATIC, nullptr, 64);
        REQUIRE(b != nullptr);
        REQUIRE(ris_buffer_get_size(b) == 64);
        ris_buffer_destroy(b);
    }
    SECTION("png and ktx2 textures") {
        ris_texture* png = ris_texture_load_image_file(RIS_TEST_DATA_DIR "/test.png", 1, 0, 0);
        REQUIRE(png != nullptr);
        REQUIRE(ris_texture_get_width(png) > 0);
        ris_destroyTexture(png);

        ris_texture* ktx = ris_texture_load_ktx2_file(RIS_TEST_DATA_DIR "/test_basis_uastc.ktx2");
        INFO(ris_get_last_error());
        REQUIRE(ktx != nullptr);
        REQUIRE(ris_texture_get_width(ktx) > 0);
        REQUIRE(ris_texture_get_gl_handle(ktx) != 0);
        ris_destroyTexture(ktx);
    }
    SECTION("render target and read back") {
        ris_render_target* rt = ris_render_target_create(16, 16, 1);
        REQUIRE(rt != nullptr);
        ris_render_target_bind(rt);
        ris_gfx_clear(1.0f, 0.0f, 0.0f, 1.0f, RIS_CLEAR_COLOR);
        uint8_t px[4] = {};
        REQUIRE(ris_gfx_read_pixels(0, 0, 1, 1, px, sizeof(px)) == RIS_OK);
        uint8_t tiny[2] = {};
        REQUIRE(ris_gfx_read_pixels(0, 0, 1, 1, tiny, sizeof(tiny)) == RIS_ERROR_INVALID_ARGUMENT);
        REQUIRE(px[0] == 255);
        REQUIRE(px[1] == 0);
        ris_render_target_bind(nullptr);
        ris_render_target_destroy(rt);
    }
    ris_window_destroy(window);
}

// ---- framework C API (ris_createFramework, listeners, ris_initialize) ---------------------------

namespace {
struct FrameworkCounts {
    int load = 0, render = 0;
    ris_framework* seenFramework = nullptr;
};
void RIS_CALL onLoad(ris_framework* f, void* userData) {
    auto* c = static_cast<FrameworkCounts*>(userData);
    ++c->load;
    c->seenFramework = f;
}
void RIS_CALL onRender(ris_framework* f, void* userData) {
    ++static_cast<FrameworkCounts*>(userData)->render;
    ris_quit(f);
}
} // namespace

TEST_CASE("framework C API defaults and NULL arguments need no display") {
    const FrameworkConfiguration config = ris_createFrameworkConfiguration();
    REQUIRE(std::strcmp(config.title, "Ris Framework") == 0);
    REQUIRE(config.backBufferWidth == 800);
    REQUIRE(config.backBufferHeight == 600);
    REQUIRE(config.textureFiltering == RIS_TEXTURE_FILTERING_TRILINEAR);

    REQUIRE(ris_createFramework(nullptr) == nullptr);
    ris_destroyFramework(nullptr);
    REQUIRE(ris_addOnLoadContentListener(nullptr, onLoad, nullptr) == 0);
    REQUIRE(ris_initialize(nullptr) == RIS_ERROR_INVALID_ARGUMENT);
}

TEST_CASE("framework C API runs the load content and render listeners") {
    if (!display_available()) {
        SKIP("no display available");
    }
    FrameworkConfiguration config = ris_createFrameworkConfiguration();
    config.backBufferWidth = 64;
    config.backBufferHeight = 64;
    ris_framework* framework = ris_createFramework(&config);
    REQUIRE(framework != nullptr);

    FrameworkCounts counts;
    REQUIRE(ris_addOnLoadContentListener(framework, onLoad, &counts) > 0);
    REQUIRE(ris_addOnRenderListener(framework, onRender, &counts) > 0);
    ris_setClearColor(ris_getRenderer(framework), 0.25f, 0.5f, 0.75f, 1.0f);
    REQUIRE(ris_getSpriteBatch(framework) != nullptr);

    REQUIRE(ris_initialize(framework) == RIS_OK);   // the render listener quits after the first frame
    REQUIRE(counts.load == 1);
    REQUIRE(counts.render == 1);
    REQUIRE(counts.seenFramework == framework);
    ris_destroyFramework(framework);
}
