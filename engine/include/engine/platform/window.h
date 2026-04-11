#pragma once

#include "engine/core/types.h"

namespace chad
{

constexpr i32 DEFAULT_WIN_WIDTH  = 1280;
constexpr i32 DEFAULT_WIN_HEIGHT = 720;

struct WindowConfig
{
    i32         width      = DEFAULT_WIN_WIDTH;
    i32         height     = DEFAULT_WIN_HEIGHT;
    const char *title      = "Chad Engine";
    bool        vsync      = false;
    bool        fullscreen = false;
    bool        maximized  = false;
};

struct Window;

Window *windowCreate(const WindowConfig &config);
void    windowDestroy(Window *window);
bool    windowShouldClose(Window *window);
void    windowSwapBuffers(Window *window);
void    windowGetSize(Window *window, i32 *width, i32 *height);
void    windowGetFramebufferSize(Window *window, i32 *width, i32 *height);
void   *windowGetNativeHandle(Window *window);  // returns GLFWwindow*

}  // namespace chad
