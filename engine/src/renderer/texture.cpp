#include <engine/renderer/texture.h>
#include <engine/core/log.h>

#include <glad/glad.h>

#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace chad
{

// Fallback texture used when requested path fails to load.
// Keep relative to working dir — game runs from repo root (bin/chad_game cwd = repo).
static const char *FALLBACK_TEXTURE = "assets/textures/Brick_0.png";

static bool s_stbi_initialized = false;

static void initStbi()
{
    if (!s_stbi_initialized) {
        stbi_set_flip_vertically_on_load(1);
        s_stbi_initialized = true;
    }
}

static GLenum channelsToFormat(i32 channels)
{
    switch (channels) {
        case 1:
            return GL_RED;
        case 2:
            return GL_RG;
        case 3:
            return GL_RGB;
        case 4:
            return GL_RGBA;
        default:
            return GL_RGB;
    }
}

static Texture uploadTexture(u8 *data, i32 width, i32 height, i32 channels)
{
    Texture tex  = {};
    tex.width    = width;
    tex.height   = height;
    tex.channels = channels;

    GLenum format = channelsToFormat(channels);

    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // PSX style: nearest everywhere, mipmaps reduce aliasing at distance
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

Texture textureLoad(const char *path)
{
    initStbi();

    i32  width    = 0;
    i32  height   = 0;
    i32  channels = 0;
    u8  *data     = stbi_load(path, &width, &height, &channels, 0);

    if (data == nullptr) {
        LOG_ERROR("Failed to load texture: %s", path);

        // Avoid infinite recursion if the fallback itself is missing
        if (std::strcmp(path, FALLBACK_TEXTURE) != 0) {
            LOG_WARN("Falling back to: %s", FALLBACK_TEXTURE);
            return textureLoad(FALLBACK_TEXTURE);
        }

        LOG_ERROR("Fallback texture missing, using 1x1 white");
        return textureCreateWhite();
    }

    Texture tex = uploadTexture(data, width, height, channels);
    stbi_image_free(data);

    LOG_INFO("Texture loaded: %s (%dx%d, %dch)", path, tex.width, tex.height, tex.channels);
    return tex;
}

Texture textureCreateWhite()
{
    Texture tex  = {};
    tex.width    = 1;
    tex.height   = 1;
    tex.channels = 4;

    u8 white[] = {255, 255, 255, 255};

    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    return tex;
}

void textureBind(const Texture &tex, u32 slot)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, tex.id);
}

void textureUnbind(u32 slot)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void textureDestroy(Texture &tex)
{
    if (tex.id != 0U) {
        glDeleteTextures(1, &tex.id);
        tex.id = 0;
    }
}

}  // namespace chad
