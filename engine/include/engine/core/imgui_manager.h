#pragma once

// ============================================================================
// ImGuiManager — Data-driven debug UI system for Chad Engine
//
// Central manager that handles ImGui lifecycle (init/frame/shutdown),
// a data-driven widget registry, retro overlay, and config persistence.
//
// Usage:
//   1. Call imguiManagerInit() after window + OpenGL context creation
//   2. Register widgets via imguiManagerAdd*() helpers
//   3. Each frame: imguiManagerNewFrame() → imguiManagerDraw() → imguiManagerRender()
//   4. Call imguiManagerShutdown() before destroying the window
//
// Keyboard shortcuts:
//   F1 — Toggle the full debug window
//   F2 — Toggle ImGui input capture (game keeps running + receiving input)
// ============================================================================

#include <engine/core/types.h>
#include <engine/core/math.h>
#include <engine/renderer/camera.h>
#include <engine/renderer/renderer.h>

#include <functional>
#include <string>
#include <vector>

struct GLFWwindow;

namespace chad
{

// ----------------------------------------------------------------------------
// Widget type enumeration — each value maps to a specific ImGui control.
// Adding a new type here + a case in the draw switch is all you need.
// ----------------------------------------------------------------------------
enum class WidgetType : u8
{
    SliderFloat,
    SliderInt,
    Checkbox,
    Color3,
    Color4,
    Combo,
    Button,
    Separator,
    Text,
    DragFloat3
};

// ----------------------------------------------------------------------------
// DebugWidget — one entry in the data-driven widget list.
//
// Stores a type tag, a label, a category (for collapsing headers), and a
// union of typed payloads.  Every widget is self-contained: the draw loop
// just iterates the vector and switches on type — no per-widget callbacks
// needed (except Button).
// ----------------------------------------------------------------------------
struct DebugWidget
{
    WidgetType  type;
    std::string label;
    std::string category;   // widgets with the same category are grouped
    std::string config_key; // key used for JSON save/load (empty = not saved)

    // Typed payload — only the relevant field is used per WidgetType.
    // Using a simple struct-of-optionals instead of std::variant to keep
    // the code C++17-compatible without pulling in heavy template machinery.
    f32 *float_ptr   = nullptr;
    i32 *int_ptr     = nullptr;
    bool *bool_ptr   = nullptr;
    f32  min_val     = 0.0F;
    f32  max_val     = 1.0F;
    const char *format = "%.2f";

    // Combo items (WidgetType::Combo)
    std::vector<std::string> combo_items;

    // Button callback (WidgetType::Button)
    std::function<void()> callback;

    // Static text (WidgetType::Text)
    std::string text;
};

// ----------------------------------------------------------------------------
// Overlay stats — filled by the game each frame so the retro overlay
// can display them without reaching back into game-side state.
// ----------------------------------------------------------------------------
struct OverlayStats
{
    i32 internal_w   = 320;
    i32 internal_h   = 240;
    f32 snap_res     = 160.0F;
    f32 color_depth  = 31.0F;
    bool dithering   = true;
    f32 fog_start    = 5.0F;
    f32 fog_end      = 40.0F;
};

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

// Lifecycle
void imguiManagerInit(GLFWwindow *window);
void imguiManagerShutdown();

// Per-frame calls (must be called in order)
void imguiManagerNewFrame();                          // ImGui::NewFrame
void imguiManagerDraw(Camera *cam, Renderer *render); // draw debug window + overlay
void imguiManagerRender();                            // ImGui::Render + draw data

// State queries
bool imguiManagerWantsInput();   // true when ImGui is capturing keyboard/mouse
bool imguiManagerIsVisible();    // true when debug window is open (F1)

// Toggle controls (called from input handling)
void imguiManagerToggleWindow();       // F1
void imguiManagerToggleInputCapture(); // F2

// Overlay stats — update each frame before imguiManagerDraw()
void imguiManagerSetOverlayStats(const OverlayStats &stats);

// --- Data-driven widget registration ---
// All Add* functions return the index of the widget for later removal/lookup.
u32 imguiManagerAddSliderFloat(const std::string &category,
                                const std::string &label,
                                f32 *value, f32 min_val, f32 max_val,
                                const char *format = "%.2f",
                                const std::string &config_key = "");

u32 imguiManagerAddSliderInt(const std::string &category,
                              const std::string &label,
                              i32 *value, i32 min_val, i32 max_val,
                              const std::string &config_key = "");

u32 imguiManagerAddCheckbox(const std::string &category,
                             const std::string &label,
                             bool *value,
                             const std::string &config_key = "");

u32 imguiManagerAddColor3(const std::string &category,
                           const std::string &label,
                           f32 *rgb,
                           const std::string &config_key = "");

u32 imguiManagerAddColor4(const std::string &category,
                           const std::string &label,
                           f32 *rgba,
                           const std::string &config_key = "");

u32 imguiManagerAddCombo(const std::string &category,
                          const std::string &label,
                          i32 *value,
                          const std::vector<std::string> &items,
                          const std::string &config_key = "");

u32 imguiManagerAddButton(const std::string &category,
                           const std::string &label,
                           std::function<void()> callback);

u32 imguiManagerAddSeparator(const std::string &category);

u32 imguiManagerAddText(const std::string &category,
                         const std::string &label,
                         const std::string &text);

u32 imguiManagerAddDragFloat3(const std::string &category,
                               const std::string &label,
                               f32 *value, f32 speed, f32 min_val, f32 max_val,
                               const std::string &config_key = "");

// Config persistence
void imguiManagerSaveConfig(const char *path = "debug_config.json");
void imguiManagerLoadConfig(const char *path = "debug_config.json");

}  // namespace chad
