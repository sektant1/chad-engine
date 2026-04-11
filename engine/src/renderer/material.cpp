#include <engine/renderer/material.h>
#include <engine/core/log.h>

namespace chad
{

void materialBind(const Material &mat)
{
    if (mat.shader == nullptr) {
        LOG_WARN("materialBind: null shader");
        return;
    }

    shaderBind(*mat.shader);

    if (mat.use_texture && mat.texture != nullptr) {
        textureBind(*mat.texture, 0);
        shaderSetInt(*mat.shader, "uTexture", 0);
        shaderSetInt(*mat.shader, "uUseTexture", 1);
    } else {
        shaderSetInt(*mat.shader, "uUseTexture", 0);
    }

    shaderSetVec4(*mat.shader, "uTintColor", mat.color.x, mat.color.y, mat.color.z, mat.color.w);
}

void materialUnbind()
{
    textureUnbind(0);
    shaderUnbind();
}

}  // namespace chad
