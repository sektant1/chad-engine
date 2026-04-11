#include <catch2/catch_test_macros.hpp>

#include <engine/platform/window.h>

#include <GLFW/glfw3.h>

using namespace chad;

// gl_test_main.cpp already initialized GLFW + a primary hidden window.
// These tests create additional windows to verify the Window API surface.
// All test windows use GLFW_VISIBLE=false via the hint set by gl_test_main;
// however WindowConfig doesn't expose visibility, so we just rely on
// fast create/destroy. On a real desktop you may see flickers.

TEST_CASE("windowCreate produces a non-null window + sane size", "[window]")
{
    WindowConfig cfg {};
    cfg.width  = 320;
    cfg.height = 240;
    cfg.title  = "chad_test_window";

    Window *w = windowCreate(cfg);
    REQUIRE(w != nullptr);

    i32 width  = 0;
    i32 height = 0;
    windowGetSize(w, &width, &height);
    REQUIRE(width > 0);
    REQUIRE(height > 0);

    // framebuffer size may differ from window size on HiDPI but must be > 0
    i32 fb_w = 0;
    i32 fb_h = 0;
    windowGetFramebufferSize(w, &fb_w, &fb_h);
    REQUIRE(fb_w > 0);
    REQUIRE(fb_h > 0);

    windowDestroy(w);
}

TEST_CASE("windowShouldClose is false on a fresh window", "[window]")
{
    WindowConfig cfg {};
    Window      *w = windowCreate(cfg);
    REQUIRE(w != nullptr);

    REQUIRE(windowShouldClose(w) == false);

    windowDestroy(w);
}

TEST_CASE("windowSwapBuffers runs without crashing", "[window]")
{
    WindowConfig cfg {};
    Window      *w = windowCreate(cfg);
    REQUIRE(w != nullptr);

    windowSwapBuffers(w);  // single swap on a visible-or-hidden context
    windowDestroy(w);
}

TEST_CASE("windowGetNativeHandle returns the underlying GLFWwindow", "[window]")
{
    WindowConfig cfg {};
    Window      *w = windowCreate(cfg);
    REQUIRE(w != nullptr);

    void *native = windowGetNativeHandle(w);
    REQUIRE(native != nullptr);
    // it's a GLFWwindow* — querying shouldClose on it should also work
    auto *glfw = static_cast<GLFWwindow *>(native);
    REQUIRE(glfwWindowShouldClose(glfw) == 0);

    windowDestroy(w);
}
