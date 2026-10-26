// Tests of WindowManager. The parts before initializeForGles() need no display.
#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <stdexcept>
#include <string>

#include <ris/window_manager.hpp>

namespace {
bool display_available() {
#if defined(__linux__)
    return std::getenv("DISPLAY") != nullptr || std::getenv("WAYLAND_DISPLAY") != nullptr;
#else
    return true;
#endif
}

ris::WindowDescriptor make_desc() {
    ris::WindowDescriptor desc;
    desc.title = "window manager test";
    desc.width = 64;
    desc.height = 48;
    return desc;
}
} // namespace

TEST_CASE("WindowManager before the window exists") {
    ris::WindowManager windowManager(make_desc());

    REQUIRE(windowManager.getTitle() == "window manager test");
    windowManager.setTitle("other");
    REQUIRE(windowManager.getTitle() == "other");

    REQUIRE_THROWS_AS(windowManager.getWindowBounds(), std::logic_error);
    REQUIRE_THROWS_AS(windowManager.swapBuffers(), std::logic_error);
    REQUIRE(windowManager.getWindow() == nullptr);
    REQUIRE_THROWS_AS(windowManager.runEventLoop(), std::logic_error);

    const auto update = windowManager.addOnUpdateListener([] {});
    const auto render = windowManager.addOnRenderListener([] {});
    const auto resize = windowManager.addOnResizeListener([](ris::WindowManager&, glm::vec2) {});
    REQUIRE(windowManager.removeOnUpdateListener(update));
    REQUIRE_FALSE(windowManager.removeOnUpdateListener(update));
    REQUIRE(windowManager.removeOnRenderListener(render));
    REQUIRE_FALSE(windowManager.removeOnRenderListener(render));
    REQUIRE(windowManager.removeOnResizeListener(resize));
    REQUIRE_FALSE(windowManager.removeOnResizeListener(resize));
}

TEST_CASE("WindowManager with a window") {
    if (!display_available()) {
        SKIP("no display available");
    }
    ris::WindowManager windowManager(make_desc());
    windowManager.initializeForGles();

    const glm::vec2 bounds = windowManager.getWindowBounds();
    REQUIRE(bounds.x > 0);
    REQUIRE(bounds.y > 0);
    REQUIRE(windowManager.getWindow() != nullptr);
    REQUIRE(windowManager.getTitle() == "window manager test");

    windowManager.setTitle("renamed");
    REQUIRE(windowManager.getTitle() == "renamed");

    SECTION("the event loop runs the update and render listeners until quit") {
        int updates = 0, renders = 0;
        windowManager.addOnUpdateListener([&] { ++updates; });
        windowManager.addOnRenderListener([&] {
            if (++renders == 3) windowManager.quit();
        });
        windowManager.runEventLoop();
        REQUIRE(updates == 3);
        REQUIRE(renders == 3);
    }
}
