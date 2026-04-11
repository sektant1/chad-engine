#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <engine/platform/timer.h>

#include <GLFW/glfw3.h>
#include <thread>
#include <chrono>

using namespace chad;
using Catch::Approx;

// Timer uses glfwGetTime which needs glfwInit. gl_test_main.cpp already
// initialized GLFW before tests run, so we can exercise the timer directly.

TEST_CASE("timerInit baseline + first delta is zero", "[timer]")
{
    timerInit();
    REQUIRE(timerGetDelta() == Approx(0.0));
    REQUIRE(timerGetElapsed() >= 0.0);
    REQUIRE(timerGetFPS() == 0);
}

TEST_CASE("timerUpdate produces a non-negative delta", "[timer]")
{
    timerInit();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    timerUpdate();

    f64 dt = timerGetDelta();
    REQUIRE(dt > 0.0);
    // sleep is at least 10ms, realistically more
    REQUIRE(dt >= 0.005);
    // sanity upper bound — test machine shouldn't stall 1s on a 10ms sleep
    REQUIRE(dt < 1.0);
}

TEST_CASE("timerGetElapsed increases monotonically", "[timer]")
{
    timerInit();
    f64 t0 = timerGetElapsed();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    f64 t1 = timerGetElapsed();
    REQUIRE(t1 > t0);
}

TEST_CASE("timer tracks multiple frames", "[timer]")
{
    timerInit();
    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        timerUpdate();
        REQUIRE(timerGetDelta() > 0.0);
    }
}
