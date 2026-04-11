#include <engine/renderer/shader.h>
#include <engine/core/log.h>

#include <glad/glad.h>

#include <fstream>
#include <sstream>
#include <string>

namespace chad
{

static std::string readFile(const char *path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        LOG_ERROR("Failed to open file: %s", path);
        return {};
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static u32 compileShader(GLenum type, const char *source)
{
    u32 shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        LOG_ERROR("Shader compilation failed: %s", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static i32 getUniformLocation(const Shader &shader, const char *name)
{
    auto it = shader.uniform_cache.find(name);
    if (it != shader.uniform_cache.end()) {
        return it->second;
    }
    i32 loc                    = glGetUniformLocation(shader.program, name);
    shader.uniform_cache[name] = loc;
    if (loc < 0) {
        LOG_WARN("Uniform '%s' not found in shader %u", name, shader.program);
    }
    return loc;
}

Shader shaderCreate(const char *vertex_src, const char *fragment_src)
{
    Shader shader = {};

    u32 vert = compileShader(GL_VERTEX_SHADER, vertex_src);
    u32 frag = compileShader(GL_FRAGMENT_SHADER, fragment_src);

    if ((vert == 0U) || (frag == 0U)) {
        if (vert != 0U) {
            glDeleteShader(vert);
        }
        if (frag != 0U) {
            glDeleteShader(frag);
        }
        return shader;
    }

    shader.program = glCreateProgram();
    glAttachShader(shader.program, vert);
    glAttachShader(shader.program, frag);
    glLinkProgram(shader.program);

    int success = 0;
    glGetProgramiv(shader.program, GL_LINK_STATUS, &success);
    if (success == 0) {
        char log[1024];
        glGetProgramInfoLog(shader.program, sizeof(log), nullptr, log);
        LOG_ERROR("Shader link failed: %s", log);
        glDeleteProgram(shader.program);
        shader.program = 0;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);

    return shader;
}

Shader shaderLoad(const char *vert_path, const char *frag_path)
{
    std::string vert_src = readFile(vert_path);
    std::string frag_src = readFile(frag_path);

    if (vert_src.empty() || frag_src.empty()) {
        return {};
    }
    return shaderCreate(vert_src.c_str(), frag_src.c_str());
}

void shaderDestroy(Shader &shader)
{
    if (shader.program != 0U) {
        glDeleteProgram(shader.program);
        shader.program = 0;
    }
    shader.uniform_cache.clear();
}

void shaderBind(const Shader &shader)
{
    glUseProgram(shader.program);
}

void shaderUnbind()
{
    glUseProgram(0);
}

void shaderSetInt(const Shader &shader, const char *name, i32 value)
{
    glUniform1i(getUniformLocation(shader, name), value);
}

void shaderSetFloat(const Shader &shader, const char *name, f32 value)
{
    glUniform1f(getUniformLocation(shader, name), value);
}

void shaderSetVec3(const Shader &shader, const char *name, f32 x, f32 y, f32 z)
{
    glUniform3f(getUniformLocation(shader, name), x, y, z);
}

void shaderSetVec4(const Shader &shader, const char *name, f32 x, f32 y, f32 z, f32 w)
{
    glUniform4f(getUniformLocation(shader, name), x, y, z, w);
}

void shaderSetMatrix4(const Shader &shader, const char *name, const Matrix4 &m)
{
    glUniformMatrix4fv(getUniformLocation(shader, name), 1, GL_FALSE, m.data.data());
}

}  // namespace chad
