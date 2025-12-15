#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

auto main() -> int
{
    if (!glfwInit()) {
        std::cout << "Failed initializing glfw" << "\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window =
        glfwCreateWindow(1280, 720, "OpenGL Game", nullptr, nullptr);

    if (window == nullptr) {
        std::cout << "Error creating window" << "\n";
        glfwTerminate();
        return -1;
    }

    glfwSetWindowPos(window, 256, 256);
    glfwMakeContextCurrent(window);

    if (!glewInit() != GLEW_OK) {
        std::cout << "Failed initializing glew" << "\n";
        glfwTerminate();
        return -1;
    }

    while (!glfwWindowShouldClose(window)) {
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}
