#include <engine/platform/window.h>
#include <engine/core/log.h>
#include <engine/core/assert.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace chad
{

struct Window
{
    GLFWwindow *handle;
    i32         width;
    i32         height;
};

static bool s_glfw_initialized = false;

static void glfwErrorCallback(int error, const char *description)
{
    LOG_ERROR("GLFW error %d: %s", error, description);
}

Window *windowCreate(const WindowConfig &config)
{
    if (!s_glfw_initialized) {
        glfwSetErrorCallback(glfwErrorCallback);
        if (glfwInit() == 0) {
            LOG_FATAL("Failed to initialize GLFW");
            return nullptr;
        }
        s_glfw_initialized = true;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_MAXIMIZED, config.maximized ? GLFW_TRUE : GLFW_FALSE);

    GLFWwindow *handle = glfwCreateWindow(config.width, config.height, config.title, nullptr, nullptr);

    if (handle == nullptr) {
        LOG_FATAL("Failed to create GLFW window");
        return nullptr;
    }

    glfwMakeContextCurrent(handle);

    if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
        LOG_FATAL("Failed to initialize glad");
        glfwDestroyWindow(handle);
        return nullptr;
    }

    glfwSwapInterval(config.vsync ? 1 : 0);

    auto *window   = new Window();
    window->handle = handle;
    window->width  = config.width;
    window->height = config.height;

    LOG_INFO("Window created: %dx%d - %s", config.width, config.height, config.title);
    LOG_INFO("OpenGL: %s", glGetString(GL_VERSION));
    LOG_INFO("Renderer: %s", glGetString(GL_RENDERER));

    return window;
}

void windowDestroy(Window *window)
{
    if (window != nullptr) {
        glfwDestroyWindow(window->handle);
        delete window;
    }
    // Do NOT glfwTerminate here — other windows (or tests) may still need
    // GLFW state. Terminate once at process shutdown via atexit, or let
    // the OS reclaim on exit.
}

bool windowShouldClose(Window *window)
{
    return glfwWindowShouldClose(window->handle) != 0;
}

void windowPollEvents(Window * /*window*/)
{
    glfwPollEvents();
}

void windowSwapBuffers(Window *window)
{
    glfwSwapBuffers(window->handle);
}

void windowGetSize(Window *window, i32 *width, i32 *height)
{
    glfwGetWindowSize(window->handle, width, height);
}

void windowGetFramebufferSize(Window *window, i32 *width, i32 *height)
{
    glfwGetFramebufferSize(window->handle, width, height);
}

void *windowGetNativeHandle(Window *window)
{
    return window->handle;
}

}  // namespace chad
