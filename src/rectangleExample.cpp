#include <iostream>
#include <string>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;
};

Vec2 offset;

void keyCallback(GLFWwindow *window, int key, int scanCode, int action, int mods)

{
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_UP:
                offset.y += 0.01f;
                std::cout << "GLFW_KEY_UP" << "\n";
                break;
            case GLFW_KEY_DOWN:
                offset.y -= 0.01f;
                std::cout << "GLFW_KEY_DOWN" << "\n";
                break;
            case GLFW_KEY_RIGHT:
                offset.x += 0.01f;
                std::cout << "GLFW_KEY_RIGHT" << "\n";
                break;
            case GLFW_KEY_LEFT:
                offset.x -= 0.01f;
                std::cout << "GLFW_KEY_LEFT" << "\n";
                break;
            default:
                break;
        }
    }
}

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

    glfwSetKeyCallback(window, keyCallback);

    // glfwSetWindowPos(window, 256, 256);
    glfwMakeContextCurrent(window);

    if (!glewInit() != GLEW_OK) {
        std::cout << "Failed initializing glew" << "\n";
        glfwTerminate();
        return -1;
    }

    // vertex shader code / runs once per triangle vertex
    std::string vertexShaderSource = R"(
		#version 330 core
		layout (location = 0) in vec3 position;
		layout (location = 1) in vec3 color;

		uniform vec2 uOffset;

		out vec3 vColor;

		void main()
		{	
			vColor = color;
			gl_Position = vec4(position.x + uOffset.x, position.y + uOffset.y, position.z, 1.0);
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

    // fragment shader code / runs once per pixel in triangle vertex
    std::string fragmentShaderSource = R"(
		#version 330 core
		out vec4 FragColor;
		
		in vec3 vColor;
		uniform vec4 uColor;

		void main()
		{
			FragColor = vec4(vColor, 1.0) * uColor;
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

    // clang-format off
    std::vector<float> vertices = {
		0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
	   -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
	   -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f
	};

	std::vector<unsigned int> indices =
	{
		0, 1, 2,
		0, 2, 3
	};
    // clang-format on

    // vertex buffer obj
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // vertex array obj
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    // grabs first 3 floats of the vector
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // grabs after 3 floats of the vector
    glVertexAttribPointer(1, 3, GL_FLOAT, false, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    GLint uColorLoc  = glGetUniformLocation(shaderProgram, "uColor");
    GLint uOffsetLoc = glGetUniformLocation(shaderProgram, "uOffset");

    while (!glfwWindowShouldClose(window)) {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);  // set buffer bg color
        glClear(GL_COLOR_BUFFER_BIT);          // clears and paint buffer

        glUseProgram(shaderProgram);  // load/binds the pipeline
        glUniform4f(uColorLoc, 0.0f, 1.0f, 0.0f, 1.0f);
        glUniform2f(uOffsetLoc, offset.x, offset.y);
        glBindVertexArray(vao);                               // tells gpu where to get data (VAO reads VBO)
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);  // draw elements
        glfwSwapBuffers(window);                              // flips buffer to the screen
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}
