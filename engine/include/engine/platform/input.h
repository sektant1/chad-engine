#pragma once

#include <engine/core/types.h>

namespace chad
{

enum class Key : u8
{
    W,
    A,
    S,
    D,
    Q,
    E,
    R,
    F,
    Space,
    LeftShift,
    LeftCtrl,
    Escape,
    Tab,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Up,
    Down,
    Left,
    Right,
    GraveAccent,
    F1,
    F2,
    F3,
    Count
};

enum class MouseButton : u8
{
    Left,
    Right,
    Middle,
    Count
};

struct Window;

void inputInit(Window *win);
void inputUpdate();  // call once per frame: polls events + samples state

bool inputKeyDown(Key key);
bool inputKeyPressed(Key key);   // true only on first frame
bool inputKeyReleased(Key key);  // true only on release frame

bool inputMouseDown(MouseButton button);
bool inputMousePressed(MouseButton button);

f32 inputMouseX();
f32 inputMouseY();
f32 inputMouseDX();
f32 inputMouseDY();
f32 inputScrollDY();

void inputSetCursorLocked(bool locked);

}  // namespace chad
