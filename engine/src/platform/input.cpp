#include <engine/platform/input.h>
#include <engine/platform/window.h>
#include <engine/core/assert.h>

#include <GLFW/glfw3.h>
#include <cstring>

namespace chad
{

static GLFWwindow *s_window = nullptr;

static bool s_keys_current[(u32)Key::Count]  = {};
static bool s_keys_previous[(u32)Key::Count] = {};

static bool s_mouse_current[(u32)MouseButton::Count]  = {};
static bool s_mouse_previous[(u32)MouseButton::Count] = {};

static f64  s_mouse_x = 0.0, s_mouse_y = 0.0;
static f64  s_mouse_last_x = 0.0, s_mouse_last_y = 0.0;
static f64  s_mouse_dx = 0.0, s_mouse_dy = 0.0;
static f64  s_scroll_dy   = 0.0;
static bool s_first_mouse = true;

// clang-format off
static int keyToGLFW(Key key) {
    switch (key) {
        case Key::W:           return GLFW_KEY_W;
        case Key::A:           return GLFW_KEY_A;
        case Key::S:           return GLFW_KEY_S;
        case Key::D:           return GLFW_KEY_D;
        case Key::Q:           return GLFW_KEY_Q;
        case Key::E:           return GLFW_KEY_E;
        case Key::R:           return GLFW_KEY_R;
        case Key::F:           return GLFW_KEY_F;
        case Key::Space:       return GLFW_KEY_SPACE;
        case Key::LeftShift:   return GLFW_KEY_LEFT_SHIFT;
        case Key::LeftCtrl:    return GLFW_KEY_LEFT_CONTROL;
        case Key::Escape:      return GLFW_KEY_ESCAPE;
        case Key::Tab:         return GLFW_KEY_TAB;
        case Key::Num1:        return GLFW_KEY_1;
        case Key::Num2:        return GLFW_KEY_2;
        case Key::Num3:        return GLFW_KEY_3;
        case Key::Num4:        return GLFW_KEY_4;
        case Key::Num5:        return GLFW_KEY_5;
        case Key::Up:          return GLFW_KEY_UP;
        case Key::Down:        return GLFW_KEY_DOWN;
        case Key::Left:        return GLFW_KEY_LEFT;
        case Key::Right:       return GLFW_KEY_RIGHT;
        case Key::GraveAccent: return GLFW_KEY_GRAVE_ACCENT;
        default:               return GLFW_KEY_UNKNOWN;
    }
}

// clang-format on

static int mouseToGLFW(MouseButton button)
{
    switch (button) {
        case MouseButton::Left:
            return GLFW_MOUSE_BUTTON_LEFT;
        case MouseButton::Right:
            return GLFW_MOUSE_BUTTON_RIGHT;
        case MouseButton::Middle:
            return GLFW_MOUSE_BUTTON_MIDDLE;
        default:
            return -1;
    }
}

static void scrollCallback(GLFWwindow *, double, double yoffset)
{
    s_scroll_dy = yoffset;
}

void inputInit(Window *win)
{
    s_window = (GLFWwindow *)windowGetNativeHandle(win);
    glfwSetScrollCallback(s_window, scrollCallback);
    glfwGetCursorPos(s_window, &s_mouse_x, &s_mouse_y);
    s_mouse_last_x = s_mouse_x;
    s_mouse_last_y = s_mouse_y;
    s_first_mouse  = true;
}

void inputUpdate()
{
    // 1. carry state forward: previous ← current
    memcpy(s_keys_previous, s_keys_current, sizeof(s_keys_current));
    memcpy(s_mouse_previous, s_mouse_current, sizeof(s_mouse_current));

    // 2. reset per-frame scroll before callback repopulates it
    s_scroll_dy = 0.0;

    // 3. drive event loop (scroll callback fires here)
    glfwPollEvents();

    // 4. sample new key state
    for (u32 i = 0; i < (u32)Key::Count; i++) {
        int glfw_key      = keyToGLFW((Key)i);
        s_keys_current[i] = (glfw_key != GLFW_KEY_UNKNOWN) && (glfwGetKey(s_window, glfw_key) == GLFW_PRESS);
    }

    // 5. sample mouse buttons
    for (u32 i = 0; i < (u32)MouseButton::Count; i++) {
        int glfw_btn       = mouseToGLFW((MouseButton)i);
        s_mouse_current[i] = (glfw_btn >= 0) && (glfwGetMouseButton(s_window, glfw_btn) == GLFW_PRESS);
    }

    // 6. sample mouse position + derive delta
    glfwGetCursorPos(s_window, &s_mouse_x, &s_mouse_y);
    if (s_first_mouse) {
        s_mouse_last_x = s_mouse_x;
        s_mouse_last_y = s_mouse_y;
        s_first_mouse  = false;
    }
    s_mouse_dx     = s_mouse_x - s_mouse_last_x;
    s_mouse_dy     = s_mouse_y - s_mouse_last_y;
    s_mouse_last_x = s_mouse_x;
    s_mouse_last_y = s_mouse_y;
}

bool inputKeyDown(Key key)
{
    return s_keys_current[(u32)key];
}

bool inputKeyPressed(Key key)
{
    return s_keys_current[(u32)key] && !s_keys_previous[(u32)key];
}

bool inputKeyReleased(Key key)
{
    return !s_keys_current[(u32)key] && s_keys_previous[(u32)key];
}

bool inputMouseDown(MouseButton button)
{
    return s_mouse_current[(u32)button];
}

bool inputMousePressed(MouseButton button)
{
    return s_mouse_current[(u32)button] && !s_mouse_previous[(u32)button];
}

f32 inputMouseX()
{
    return (f32)s_mouse_x;
}

f32 inputMouseY()
{
    return (f32)s_mouse_y;
}

f32 inputMouseDX()
{
    return (f32)s_mouse_dx;
}

f32 inputMouseDY()
{
    return (f32)s_mouse_dy;
}

f32 inputScrollDY()
{
    return (f32)s_scroll_dy;
}

void inputSetCursorLocked(bool locked)
{
    glfwSetInputMode(s_window, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (locked) {
        s_first_mouse = true;
    }
}

}  // namespace chad
