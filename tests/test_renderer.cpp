#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <engine/renderer/renderer.h>

#include <glad/glad.h>

using namespace chad;

// These tests require a GL context (chad_gl_tests binary).

TEST_CASE("rendererCreate returns valid pointer with default config", "[renderer]")
{
    RendererConfig config {};
    Renderer      *r = rendererCreate(config);
    REQUIRE(r != nullptr);
    rendererDestroy(r);
}

TEST_CASE("rendererCreate with custom resolution", "[renderer]")
{
    RendererConfig config {};
    config.internal_width  = 640;
    config.internal_height = 480;

    Renderer *r = rendererCreate(config);
    REQUIRE(r != nullptr);

    i32 w = 0;
    i32 h = 0;
    rendererGetInternalSize(r, &w, &h);
    REQUIRE(w == 640);
    REQUIRE(h == 480);

    rendererDestroy(r);
}

TEST_CASE("rendererGetInternalSize returns default PSX resolution", "[renderer]")
{
    RendererConfig config {};
    Renderer      *r = rendererCreate(config);

    i32 w = 0;
    i32 h = 0;
    rendererGetInternalSize(r, &w, &h);
    REQUIRE(w == 320);
    REQUIRE(h == 240);

    rendererDestroy(r);
}

TEST_CASE("rendererResize updates internal size", "[renderer]")
{
    RendererConfig config {};
    Renderer      *r = rendererCreate(config);

    rendererResize(r, 512, 384);

    i32 w = 0;
    i32 h = 0;
    rendererGetInternalSize(r, &w, &h);
    REQUIRE(w == 512);
    REQUIRE(h == 384);

    rendererDestroy(r);
}

TEST_CASE("rendererResize ignores zero/negative dimensions", "[renderer]")
{
    RendererConfig config {};
    Renderer      *r = rendererCreate(config);

    rendererResize(r, 0, 0);
    i32 w = 0;
    i32 h = 0;
    rendererGetInternalSize(r, &w, &h);
    REQUIRE(w == 320);
    REQUIRE(h == 240);

    rendererResize(r, -1, 100);
    rendererGetInternalSize(r, &w, &h);
    REQUIRE(w == 320);
    REQUIRE(h == 240);

    rendererDestroy(r);
}

TEST_CASE("rendererResize same size is no-op", "[renderer]")
{
    RendererConfig config {};
    Renderer      *r = rendererCreate(config);

    // Should not crash or change anything
    rendererResize(r, 320, 240);

    i32 w = 0;
    i32 h = 0;
    rendererGetInternalSize(r, &w, &h);
    REQUIRE(w == 320);
    REQUIRE(h == 240);

    rendererDestroy(r);
}

TEST_CASE("rendererBeginFrame/EndFrame cycle does not crash", "[renderer]")
{
    RendererConfig config {};
    Renderer      *r = rendererCreate(config);

    rendererBeginFrame(r);
    rendererEndFrame(r);

    rendererDestroy(r);
}

TEST_CASE("rendererPresent does not crash", "[renderer]")
{
    RendererConfig config {};
    Renderer      *r = rendererCreate(config);

    rendererBeginFrame(r);
    rendererEndFrame(r);
    rendererPresent(r, 800, 600);

    rendererDestroy(r);
}

TEST_CASE("rendererBeginFrame binds FBO (not default framebuffer)", "[renderer]")
{
    RendererConfig config {};
    Renderer      *r = rendererCreate(config);

    rendererBeginFrame(r);

    GLint current_fbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &current_fbo);
    REQUIRE(current_fbo != 0);  // should be bound to FBO, not default

    rendererEndFrame(r);

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &current_fbo);
    REQUIRE(current_fbo == 0);  // back to default

    rendererDestroy(r);
}

TEST_CASE("rendererSetClearColor changes GL state", "[renderer]")
{
    rendererSetClearColor(0.5F, 0.25F, 0.75F, 1.0F);

    GLfloat color[4] = {};
    glGetFloatv(GL_COLOR_CLEAR_VALUE, color);
    REQUIRE(color[0] == Catch::Approx(0.5F));
    REQUIRE(color[1] == Catch::Approx(0.25F));
    REQUIRE(color[2] == Catch::Approx(0.75F));
    REQUIRE(color[3] == Catch::Approx(1.0F));
}

TEST_CASE("rendererSetViewport changes GL state", "[renderer]")
{
    rendererSetViewport(10, 20, 300, 200);

    GLint viewport[4] = {};
    glGetIntegerv(GL_VIEWPORT, viewport);
    REQUIRE(viewport[0] == 10);
    REQUIRE(viewport[1] == 20);
    REQUIRE(viewport[2] == 300);
    REQUIRE(viewport[3] == 200);
}

TEST_CASE("rendererDestroy on nullptr is safe", "[renderer]")
{
    rendererDestroy(nullptr);  // should not crash
}

TEST_CASE("multiple renderers can coexist", "[renderer]")
{
    RendererConfig config_a {};
    config_a.internal_width  = 320;
    config_a.internal_height = 240;

    RendererConfig config_b {};
    config_b.internal_width  = 640;
    config_b.internal_height = 480;

    Renderer *a = rendererCreate(config_a);
    Renderer *b = rendererCreate(config_b);
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);

    i32 w = 0;
    i32 h = 0;
    rendererGetInternalSize(a, &w, &h);
    REQUIRE(w == 320);

    rendererGetInternalSize(b, &w, &h);
    REQUIRE(w == 640);

    rendererDestroy(a);
    rendererDestroy(b);
}
