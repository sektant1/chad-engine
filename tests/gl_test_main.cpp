// Custom Catch2 main that initializes a hidden OpenGL 3.3 context before
// running tests. All tests share the same context to avoid repeated window
// creation overhead.

#define CATCH_CONFIG_RUNNER
#include <catch2/catch_session.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>

static GLFWwindow *g_window = nullptr;

static bool initGL()
{
    if (glfwInit() == 0) {
        std::fprintf(stderr, "[test] glfwInit failed\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);  // hidden — tests headless

    g_window = glfwCreateWindow(64, 64, "chad_tests", nullptr, nullptr);
    if (g_window == nullptr) {
        std::fprintf(stderr, "[test] glfwCreateWindow failed — no display?\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(g_window);
    if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
        std::fprintf(stderr, "[test] gladLoadGLLoader failed\n");
        glfwDestroyWindow(g_window);
        glfwTerminate();
        return false;
    }

    return true;
}

static void shutdownGL()
{
    if (g_window != nullptr) {
        glfwDestroyWindow(g_window);
        g_window = nullptr;
    }
    glfwTerminate();
}

int main(int argc, char *argv[])
{
    if (!initGL()) {
        // Skip with code 77 — CTest convention for "skipped"
        std::fprintf(stderr, "[test] Skipping: no usable OpenGL context\n");
        return 77;
    }

    int result = Catch::Session().run(argc, argv);
    shutdownGL();
    return result;
}
