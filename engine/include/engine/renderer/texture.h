#pragma once

#include <engine/core/types.h>

namespace chad
{

struct Texture
{
    u32 id;
    i32 width;
    i32 height;
    i32 channels;
};

// Load texture with GL_NEAREST filtering (PSX style)
Texture textureLoad(const char *path);

// Create 1x1 white texture (fallback)
Texture textureCreateWhite();

void textureBind(const Texture &t, u32 slot);
void textureUnbind(u32 slot);
void textureDestroy(Texture &t);

}  // namespace chad
