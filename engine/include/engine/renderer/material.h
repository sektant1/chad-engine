#pragma once

#include <engine/core/types.h>
#include <engine/core/math.h>
#include <engine/renderer/shader.h>
#include <engine/renderer/texture.h>

namespace chad
{

// Material references a shader + texture — it does NOT own them. Lifetime
// of the underlying GL resources is managed by the caller. Multiple
// materials may share the same shader and/or texture.
struct Material
{
    const Shader  *shader  = nullptr;
    const Texture *texture = nullptr;
    Vec4           color {1, 1, 1, 1};  // tint multiplier
    bool           use_texture = true;  // false = vertex color only (very PSX)
};

inline Material materialCreate(const Shader *shader, const Texture *texture)
{
    Material m {};
    m.shader      = shader;
    m.texture     = texture;
    m.color       = {1, 1, 1, 1};
    m.use_texture = true;
    return m;
}

inline Material materialCreateColored(const Shader *shader, Vec4 color)
{
    Material m {};
    m.shader      = shader;
    m.texture     = nullptr;
    m.color       = color;
    m.use_texture = false;
    return m;
}

void materialBind(const Material &mat);
void materialUnbind();

}  // namespace chad
