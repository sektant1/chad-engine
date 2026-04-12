#include <engine/renderer/renderer.h>
#include <engine/core/log.h>

#include <glad/glad.h>

namespace chad
{

struct Renderer
{
    u32 fbo;          // framebuffer object (PSX resolution)
    u32 fbo_texture;  // color attachment
    u32 rbo;          // depth/stencil renderbuffer
    u32 screen_vao;   // fullscreen quad VAO
    u32 screen_vbo;
    i32 internal_w;
    i32 internal_h;
};

static void createFullscreenQuad(Renderer *render)
{
    // Fullscreen quad for FBO upscale
    float quad[] = {
        // pos      texcoord
        -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f,
    };

    glGenVertexArrays(1, &render->screen_vao);
    glGenBuffers(1, &render->screen_vbo);
    glBindVertexArray(render->screen_vao);
    glBindBuffer(GL_ARRAY_BUFFER, render->screen_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

Renderer *rendererCreate(const RendererConfig &config)
{
    Renderer *render   = new Renderer();
    render->internal_w = config.internal_width;
    render->internal_h = config.internal_height;

    // Create FBO for PSX internal resolution
    glGenFramebuffers(1, &render->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, render->fbo);

    // Color texture
    glGenTextures(1, &render->fbo_texture);
    glBindTexture(GL_TEXTURE_2D, render->fbo_texture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB, render->internal_w, render->internal_h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render->fbo_texture, 0);

    // Depth/stencil renderbuffer
    glGenRenderbuffers(1, &render->rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, render->rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, render->internal_w, render->internal_h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, render->rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Framebuffer is not complete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    createFullscreenQuad(render);

    glEnable(GL_DEPTH_TEST);

    LOG_INFO("Renderer created: internal %dx%d", render->internal_w, render->internal_h);

    return render;
}

void rendererDestroy(Renderer *render)
{
    if (render != nullptr) {
        glDeleteFramebuffers(1, &render->fbo);
        glDeleteTextures(1, &render->fbo_texture);
        glDeleteRenderbuffers(1, &render->rbo);
        glDeleteVertexArrays(1, &render->screen_vao);
        glDeleteBuffers(1, &render->screen_vbo);
        delete render;
    }
}

void rendererBeginFrame(Renderer *render)
{
    // Render to PSX-resolution FBO
    glBindFramebuffer(GL_FRAMEBUFFER, render->fbo);
    glViewport(0, 0, render->internal_w, render->internal_h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void rendererEndFrame(Renderer *render)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void rendererPresent(Renderer *render, i32 window_width, i32 window_height)
{
    // Blit FBO to screen with nearest-neighbor (pixelated upscale)
    glBindFramebuffer(GL_READ_FRAMEBUFFER, render->fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0,
                      0,
                      render->internal_w,
                      render->internal_h,
                      0,
                      0,
                      window_width,
                      window_height,
                      GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void rendererResize(Renderer *render, i32 width, i32 height)
{
    if (width == render->internal_w && height == render->internal_h) {
        return;
    }
    if (width < 1 || height < 1) {
        return;
    }

    render->internal_w = width;
    render->internal_h = height;

    // Resize FBO color texture
    glBindTexture(GL_TEXTURE_2D, render->fbo_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    // Resize depth/stencil renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, render->rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    LOG_INFO("Renderer resized: %dx%d", width, height);
}

void rendererGetInternalSize(Renderer *render, i32 *width, i32 *height)
{
    *width  = render->internal_w;
    *height = render->internal_h;
}

void rendererSetClearColor(f32 red, f32 green, f32 blue, f32 alpha)
{
    glClearColor(red, green, blue, alpha);
}

void rendererSetViewport(i32 x, i32 y, i32 width, i32 height)
{
    glViewport(x, y, width, height);
}

}  // namespace chad
