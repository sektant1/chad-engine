#pragma once

#include <engine/core/types.h>
#include <engine/core/math.h>

#include <string>
#include <unordered_map>

namespace chad
{

struct Shader
{
    u32 program = 0;
    // cache uniform locations to skip glGetUniformLocation on every set
    mutable std::unordered_map<std::string, i32> uniform_cache;
};

Shader shaderCreate(const char *vertex_src, const char *fragment_src);
Shader shaderLoad(const char *vert_path, const char *frag_path);
void   shaderDestroy(Shader &shader);
void   shaderBind(const Shader &shader);
void   shaderUnbind();

void shaderSetInt(const Shader &shader, const char *name, i32 value);
void shaderSetFloat(const Shader &shader, const char *name, f32 value);
void shaderSetVec3(const Shader &shader, const char *name, f32 x, f32 y, f32 z);
void shaderSetVec4(const Shader &shader, const char *name, f32 x, f32 y, f32 z, f32 w);
void shaderSetMatrix4(const Shader &shader, const char *name, const Matrix4 &m);

}  // namespace chad
