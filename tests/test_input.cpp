#include <catch2/catch_test_macros.hpp>

#include <engine/platform/input.h>
#include <engine/platform/window.h>

using namespace chad;

// Input tests can only verify initial/idle state. We cannot synthesize
// real key events in a headless test environment — glfwGetKey polls
// hardware state and nothing is pressing anything. Still useful to
// verify: init works, update is safe, no keys report as down, deltas zero.

static Window *makeTestWindow()
{
    WindowConfig cfg {};
    cfg.width  = 320;
    cfg.height = 240;
    cfg.title  = "chad_test_input";
    return windowCreate(cfg);
}

TEST_CASE("inputInit + inputUpdate on a fresh window", "[input]")
{
    Window *w = makeTestWindow();
    REQUIRE(w != nullptr);

    inputInit(w);
    inputUpdate();

    windowDestroy(w);
}

TEST_CASE("no keys reported as down on idle input", "[input]")
{
    Window *w = makeTestWindow();
    REQUIRE(w != nullptr);

    inputInit(w);
    inputUpdate();

    REQUIRE(inputKeyDown(Key::W) == false);
    REQUIRE(inputKeyDown(Key::Escape) == false);
    REQUIRE(inputKeyDown(Key::Space) == false);

    // pressed/released should also be false without transitions
    REQUIRE(inputKeyPressed(Key::W) == false);
    REQUIRE(inputKeyReleased(Key::W) == false);

    windowDestroy(w);
}

TEST_CASE("no mouse buttons reported as down on idle input", "[input]")
{
    Window *w = makeTestWindow();
    REQUIRE(w != nullptr);

    inputInit(w);
    inputUpdate();

    REQUIRE(inputMouseDown(MouseButton::Left) == false);
    REQUIRE(inputMouseDown(MouseButton::Right) == false);
    REQUIRE(inputMouseDown(MouseButton::Middle) == false);
    REQUIRE(inputMousePressed(MouseButton::Left) == false);

    windowDestroy(w);
}

TEST_CASE("mouse delta is zero after first update", "[input]")
{
    Window *w = makeTestWindow();
    REQUIRE(w != nullptr);

    inputInit(w);
    inputUpdate();  // first update primes s_first_mouse → delta forced to 0
    inputUpdate();  // second update: cursor hasn't moved in headless test

    REQUIRE(inputMouseDX() == 0.0F);
    REQUIRE(inputMouseDY() == 0.0F);
    REQUIRE(inputScrollDY() == 0.0F);

    windowDestroy(w);
}

TEST_CASE("inputSetCursorLocked toggles without crash", "[input]")
{
    Window *w = makeTestWindow();
    REQUIRE(w != nullptr);

    inputInit(w);
    inputSetCursorLocked(true);
    inputUpdate();
    inputSetCursorLocked(false);
    inputUpdate();

    windowDestroy(w);
}
