#pragma once

#include <engine/core/types.h>

namespace chad
{

struct RendererConfig
{
    i32 internal_width  = 320;
    i32 internal_height = 240;
};

struct Renderer;

Renderer *rendererCreate(const RendererConfig &config);
void      rendererDestroy(Renderer *render);

void rendererBeginFrame(Renderer *render);
void rendererEndFrame(Renderer *render);
void rendererPresent(Renderer *render, i32 window_width, i32 window_height);

void rendererResize(Renderer *render, i32 width, i32 height);
void rendererGetInternalSize(Renderer *render, i32 *width, i32 *height);
void rendererSetClearColor(f32 red, f32 green, f32 blue, f32 alpha);
void rendererSetViewport(i32 x, i32 y, i32 width, i32 height);

}  // namespace chad
