#include <iostream>
#include <string>
#include <vector>

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

    GLFWwindow *window = glfwCreateWindow(1280, 720, "OpenGL Game", nullptr, nullptr);

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

    std::string vertexShaderSource = R"(
		#version 330 core
		layout (location = 0) in vec3 position;

		void main()
		{
			gl_Position = vec4(position.x, position.y, position.z, 1.0);
		}
	)";

    GLuint      vertexShader     = glCreateShader(GL_VERTEX_SHADER);
    const char *vertexShaderCStr = vertexShaderSource.c_str();

    glShaderSource(vertexShader, 1, &vertexShaderCStr, nullptr);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "ERROR:VERTEX_SHADER_COMPILATION_FAILED: " << infoLog << "\n";
    }

    std::string fragmentShaderSource = R"(
		#version 330 core
		out vec4 FragColor;

		void main()
		{
			FragColor = vec4(1.0, 0.0, 0.0, 1.0);
		}
	)";

    GLuint      fragmentShader           = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fragmentShaderSourceCStr = fragmentShaderSource.c_str();

    glShaderSource(fragmentShader, 1, &fragmentShaderSourceCStr, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "ERROR:FRAGMENT_SHADER_COMPILATION_FAILED: " << infoLog << "\n";
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "ERROR:SHADER_PROGRAM_LINKING_FAILED: " << infoLog << "\n";
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::vector<float> vertices = {0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.f};

    while (!glfwWindowShouldClose(window)) {
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}
