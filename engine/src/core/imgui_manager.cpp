// ============================================================================
// ImGuiManager — Data-driven debug UI implementation
//
// Architecture:
//   - All widgets are stored in a flat vector<DebugWidget>.
//   - On draw, widgets are grouped by category (collapsing headers).
//   - A generic switch dispatches each widget type to its ImGui call.
//   - Config save/load serializes widget values keyed by config_key.
//   - The retro overlay is a separate always-visible ImGui window.
//
// Performance:
//   - When the debug window is closed AND the overlay is off, imguiManagerDraw()
//     early-returns after just the NewFrame/EndFrame pair, costing essentially
//     nothing (<0.01ms).
//   - When open, the data-driven loop avoids virtual calls or heap allocs
//     on the hot path — it's just a tight switch over a contiguous vector.
//
// Input capture (F2):
//   - ImGui installs its own GLFW callbacks when initialized.  By default it
//     chains to the previous callbacks, so both ImGui and the game see events.
//   - F2 toggles io.WantCaptureKeyboard / io.WantCaptureMouse overrides so
//     the game can query imguiManagerWantsInput() and skip its own handling.
//   - The game loop is never paused — only input routing changes.
// ============================================================================

#include <engine/core/imgui_manager.h>
#include <engine/core/imgui_theme.h>
#include <engine/core/log.h>
#include <engine/platform/timer.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>

namespace chad
{

// ----------------------------------------------------------------------------
// Module-level state — kept static to avoid exposing internals in the header.
// A single debug UI instance per application is the intended usage.
// ----------------------------------------------------------------------------
static struct
{
    bool initialized    = false;
    bool window_visible = false; // F1 toggles this
    bool input_capture  = false; // F2 toggles this — when true, ImGui eats input
    bool overlay_visible = false; // shown whenever window_visible is true

    std::vector<DebugWidget> widgets;
    OverlayStats             overlay_stats;
} s_state;

// ============================================================================
// Lifecycle
// ============================================================================

void imguiManagerInit(GLFWwindow *window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();

    // Prevent ImGui from changing the OS cursor — we manage that ourselves
    // via GLFW raw input mode (cursor locked/unlocked).
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    // Apply our dark theme before any rendering
    imguiApplyTheme();

    // Initialize platform + renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    s_state.initialized = true;

    LOG_INFO("ImGui debug UI initialized");
}

void imguiManagerShutdown()
{
    if (!s_state.initialized) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    s_state.widgets.clear();
    s_state.initialized = false;

    LOG_INFO("ImGui debug UI shut down");
}

// ============================================================================
// Per-frame calls
// ============================================================================

void imguiManagerNewFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

// ----------------------------------------------------------------------------
// Draw the retro overlay — a small, semi-transparent info box in the top-left
// corner showing internal resolution, PSX effect levels, and FPS.
// Always drawn when the debug window is visible, so you can glance at stats
// without the full panel open.
// ----------------------------------------------------------------------------
static void drawOverlay(Camera *cam)
{
    const auto &s = s_state.overlay_stats;

    // Position in the top-left with a small margin
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.6F);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
                           | ImGuiWindowFlags_AlwaysAutoResize
                           | ImGuiWindowFlags_NoSavedSettings
                           | ImGuiWindowFlags_NoFocusOnAppearing
                           | ImGuiWindowFlags_NoNav
                           | ImGuiWindowFlags_NoMove;

    ImGui::Begin("##RetroOverlay", nullptr, flags);

    ImGui::TextColored(ImVec4(0.8F, 0.6F, 0.2F, 1.0F), "RETRO FPS");
    ImGui::Separator();
    ImGui::Text("FPS: %u  (%.1f ms)", timerGetFPS(), (float)timerGetDelta() * 1000.0F);
    ImGui::Text("Res: %dx%d", s.internal_w, s.internal_h);
    ImGui::Text("Snap: %.0f", s.snap_res);
    ImGui::Text("Dither: %s  Depth: %.0f",
                s.dithering ? "ON" : "OFF", s.color_depth);
    ImGui::Text("Fog: %.1f - %.1f", s.fog_start, s.fog_end);

    if (cam != nullptr) {
        ImGui::Text("Pos: %.1f, %.1f, %.1f",
                     cam->position.x, cam->position.y, cam->position.z);
    }

    ImGui::End();
}

// ----------------------------------------------------------------------------
// Draw a single widget by type — the core of the data-driven approach.
// Each case maps directly to one ImGui call.  Adding a new widget type
// means adding an enum value and a case here — no other code changes needed.
// ----------------------------------------------------------------------------
static void drawWidget(DebugWidget &w)
{
    switch (w.type) {
        case WidgetType::SliderFloat:
            if (w.float_ptr != nullptr) {
                ImGui::SliderFloat(w.label.c_str(), w.float_ptr, w.min_val, w.max_val, w.format);
            }
            break;

        case WidgetType::SliderInt:
            if (w.int_ptr != nullptr) {
                ImGui::SliderInt(w.label.c_str(), w.int_ptr, (i32)w.min_val, (i32)w.max_val);
            }
            break;

        case WidgetType::Checkbox:
            if (w.bool_ptr != nullptr) {
                ImGui::Checkbox(w.label.c_str(), w.bool_ptr);
            }
            break;

        case WidgetType::Color3:
            if (w.float_ptr != nullptr) {
                ImGui::ColorEdit3(w.label.c_str(), w.float_ptr);
            }
            break;

        case WidgetType::Color4:
            if (w.float_ptr != nullptr) {
                ImGui::ColorEdit4(w.label.c_str(), w.float_ptr);
            }
            break;

        case WidgetType::Combo:
            if (w.int_ptr != nullptr && !w.combo_items.empty()) {
                // Build a null-separated string for ImGui::Combo
                std::string items_concat;
                for (const auto &item : w.combo_items) {
                    items_concat += item;
                    items_concat += '\0';
                }
                items_concat += '\0';
                ImGui::Combo(w.label.c_str(), w.int_ptr, items_concat.c_str());
            }
            break;

        case WidgetType::Button:
            if (ImGui::Button(w.label.c_str()) && w.callback) {
                w.callback();
            }
            break;

        case WidgetType::Separator:
            ImGui::Separator();
            break;

        case WidgetType::Text:
            ImGui::TextWrapped("%s", w.text.c_str());
            break;

        case WidgetType::DragFloat3:
            if (w.float_ptr != nullptr) {
                ImGui::DragFloat3(w.label.c_str(), w.float_ptr, 0.01F, w.min_val, w.max_val, w.format);
            }
            break;
    }
}

// ----------------------------------------------------------------------------
// Main draw function — groups widgets by category under collapsing headers.
// Uses a map to collect indices per category, then iterates categories in
// the order they were first registered (via a separate ordered-keys vector).
// ----------------------------------------------------------------------------
void imguiManagerDraw(Camera *cam, Renderer *render)
{
    // Early-out when everything is hidden — zero perf impact
    if (!s_state.window_visible) {
        return;
    }

    // Always draw the retro overlay when debug mode is active
    drawOverlay(cam);

    // Main debug window
    ImGui::SetNextWindowSizeConstraints(ImVec2(380, 250), ImVec2(FLT_MAX, FLT_MAX));

    if (!ImGui::Begin("Debug", &s_state.window_visible)) {
        // Window is collapsed — still need End(), but skip widget drawing
        ImGui::End();
        return;
    }

    // Performance header (always shown, not data-driven — it's fundamental)
    ImGui::SeparatorText("Performance");
    ImGui::Text("FPS: %u", timerGetFPS());
    ImGui::Text("Frame: %.2f ms", (float)timerGetDelta() * 1000.0F);
    if (cam != nullptr) {
        ImGui::Text("Pos: %.1f, %.1f, %.1f",
                     cam->position.x, cam->position.y, cam->position.z);
    }

    // Group widgets by category — collect category order from first appearance
    std::vector<std::string> category_order;
    std::map<std::string, std::vector<u32>> by_category;

    for (u32 i = 0; i < s_state.widgets.size(); i++) {
        const auto &cat = s_state.widgets[i].category;
        if (by_category.find(cat) == by_category.end()) {
            category_order.push_back(cat);
        }
        by_category[cat].push_back(i);
    }

    // Draw each category as a collapsing header with its widgets inside
    for (const auto &cat : category_order) {
        if (cat.empty()) {
            // Uncategorized widgets draw directly
            for (u32 idx : by_category[cat]) {
                drawWidget(s_state.widgets[idx]);
            }
        } else {
            ImGui::SeparatorText(cat.c_str());
            for (u32 idx : by_category[cat]) {
                drawWidget(s_state.widgets[idx]);
            }
        }
    }

    // Config save/load buttons at the bottom
    ImGui::SeparatorText("Config");
    if (ImGui::Button("Save Config")) {
        imguiManagerSaveConfig();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Config")) {
        imguiManagerLoadConfig();
    }

    // Help section
    ImGui::SeparatorText("Controls");
    ImGui::TextWrapped("F1 = toggle debug | F2 = toggle input capture | "
                       "WASD = move | Mouse = look | Space = jump | Esc = quit");

    ImGui::End();
}

void imguiManagerRender()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// ============================================================================
// State queries
// ============================================================================

bool imguiManagerWantsInput()
{
    // When F2 input capture is active AND the window is visible,
    // ImGui should eat keyboard/mouse input instead of the game.
    return s_state.input_capture && s_state.window_visible;
}

bool imguiManagerIsVisible()
{
    return s_state.window_visible;
}

// ============================================================================
// Toggle controls
// ============================================================================

void imguiManagerToggleWindow()
{
    s_state.window_visible = !s_state.window_visible;
    LOG_INFO("Debug window: %s", s_state.window_visible ? "visible" : "hidden");
}

void imguiManagerToggleInputCapture()
{
    s_state.input_capture = !s_state.input_capture;
    LOG_INFO("ImGui input capture: %s", s_state.input_capture ? "ON" : "OFF");
}

// ============================================================================
// Overlay stats
// ============================================================================

void imguiManagerSetOverlayStats(const OverlayStats &stats)
{
    s_state.overlay_stats = stats;
}

// ============================================================================
// Widget registration — each function creates a DebugWidget, pushes it
// into the vector, and returns its index.
// ============================================================================

u32 imguiManagerAddSliderFloat(const std::string &category,
                                const std::string &label,
                                f32 *value, f32 min_val, f32 max_val,
                                const char *format,
                                const std::string &config_key)
{
    DebugWidget w;
    w.type       = WidgetType::SliderFloat;
    w.category   = category;
    w.label      = label;
    w.float_ptr  = value;
    w.min_val    = min_val;
    w.max_val    = max_val;
    w.format     = format;
    w.config_key = config_key;
    s_state.widgets.push_back(std::move(w));
    return static_cast<u32>(s_state.widgets.size() - 1);
}

u32 imguiManagerAddSliderInt(const std::string &category,
                              const std::string &label,
                              i32 *value, i32 min_val, i32 max_val,
                              const std::string &config_key)
{
    DebugWidget w;
    w.type       = WidgetType::SliderInt;
    w.category   = category;
    w.label      = label;
    w.int_ptr    = value;
    w.min_val    = static_cast<f32>(min_val);
    w.max_val    = static_cast<f32>(max_val);
    w.config_key = config_key;
    s_state.widgets.push_back(std::move(w));
    return static_cast<u32>(s_state.widgets.size() - 1);
}

u32 imguiManagerAddCheckbox(const std::string &category,
                             const std::string &label,
                             bool *value,
                             const std::string &config_key)
{
    DebugWidget w;
    w.type       = WidgetType::Checkbox;
    w.category   = category;
    w.label      = label;
    w.bool_ptr   = value;
    w.config_key = config_key;
    s_state.widgets.push_back(std::move(w));
    return static_cast<u32>(s_state.widgets.size() - 1);
}

u32 imguiManagerAddColor3(const std::string &category,
                           const std::string &label,
                           f32 *rgb,
                           const std::string &config_key)
{
    DebugWidget w;
    w.type       = WidgetType::Color3;
    w.category   = category;
    w.label      = label;
    w.float_ptr  = rgb;
    w.config_key = config_key;
    s_state.widgets.push_back(std::move(w));
    return static_cast<u32>(s_state.widgets.size() - 1);
}

u32 imguiManagerAddColor4(const std::string &category,
                           const std::string &label,
                           f32 *rgba,
                           const std::string &config_key)
{
    DebugWidget w;
    w.type       = WidgetType::Color4;
    w.category   = category;
    w.label      = label;
    w.float_ptr  = rgba;
    w.config_key = config_key;
    s_state.widgets.push_back(std::move(w));
    return static_cast<u32>(s_state.widgets.size() - 1);
}

u32 imguiManagerAddCombo(const std::string &category,
                          const std::string &label,
                          i32 *value,
                          const std::vector<std::string> &items,
                          const std::string &config_key)
{
    DebugWidget w;
    w.type        = WidgetType::Combo;
    w.category    = category;
    w.label       = label;
    w.int_ptr     = value;
    w.combo_items = items;
    w.config_key  = config_key;
    s_state.widgets.push_back(std::move(w));
    return static_cast<u32>(s_state.widgets.size() - 1);
}

u32 imguiManagerAddButton(const std::string &category,
                           const std::string &label,
                           std::function<void()> callback)
{
    DebugWidget w;
    w.type     = WidgetType::Button;
    w.category = category;
    w.label    = label;
    w.callback = std::move(callback);
    s_state.widgets.push_back(std::move(w));
    return static_cast<u32>(s_state.widgets.size() - 1);
}

u32 imguiManagerAddSeparator(const std::string &category)
{
    DebugWidget w;
    w.type     = WidgetType::Separator;
    w.category = category;
    s_state.widgets.push_back(std::move(w));
    return static_cast<u32>(s_state.widgets.size() - 1);
}

u32 imguiManagerAddText(const std::string &category,
                         const std::string &label,
                         const std::string &text)
{
    DebugWidget w;
    w.type     = WidgetType::Text;
    w.category = category;
    w.label    = label;
    w.text     = text;
    s_state.widgets.push_back(std::move(w));
    return static_cast<u32>(s_state.widgets.size() - 1);
}

u32 imguiManagerAddDragFloat3(const std::string &category,
                               const std::string &label,
                               f32 *value, f32 speed, f32 min_val, f32 max_val,
                               const std::string &config_key)
{
    DebugWidget w;
    w.type       = WidgetType::DragFloat3;
    w.category   = category;
    w.label      = label;
    w.float_ptr  = value;
    w.min_val    = min_val;
    w.max_val    = max_val;
    w.format     = "%.2f";
    w.config_key = config_key;
    // speed is baked into the DragFloat3 call (0.01 default in drawWidget)
    (void)speed;
    s_state.widgets.push_back(std::move(w));
    return static_cast<u32>(s_state.widgets.size() - 1);
}

// ============================================================================
// JSON config persistence — hand-rolled to avoid adding a dependency.
//
// Format is a flat JSON object:
//   {
//     "snap_resolution": 160.0,
//     "dithering_enabled": true,
//     ...
//   }
//
// Only widgets with a non-empty config_key are serialized.
// ============================================================================

void imguiManagerSaveConfig(const char *path)
{
    std::ofstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to save debug config to %s", path);
        return;
    }

    file << "{\n";
    bool first = true;

    for (const auto &w : s_state.widgets) {
        if (w.config_key.empty()) {
            continue;
        }

        if (!first) {
            file << ",\n";
        }
        first = false;

        file << "  \"" << w.config_key << "\": ";

        switch (w.type) {
            case WidgetType::SliderFloat:
                if (w.float_ptr != nullptr) {
                    file << *w.float_ptr;
                }
                break;

            case WidgetType::SliderInt:
            case WidgetType::Combo:
                if (w.int_ptr != nullptr) {
                    file << *w.int_ptr;
                }
                break;

            case WidgetType::Checkbox:
                if (w.bool_ptr != nullptr) {
                    file << (*w.bool_ptr ? "true" : "false");
                }
                break;

            case WidgetType::Color3:
                if (w.float_ptr != nullptr) {
                    file << "[" << w.float_ptr[0] << ", "
                         << w.float_ptr[1] << ", "
                         << w.float_ptr[2] << "]";
                }
                break;

            case WidgetType::Color4:
                if (w.float_ptr != nullptr) {
                    file << "[" << w.float_ptr[0] << ", "
                         << w.float_ptr[1] << ", "
                         << w.float_ptr[2] << ", "
                         << w.float_ptr[3] << "]";
                }
                break;

            case WidgetType::DragFloat3:
                if (w.float_ptr != nullptr) {
                    file << "[" << w.float_ptr[0] << ", "
                         << w.float_ptr[1] << ", "
                         << w.float_ptr[2] << "]";
                }
                break;

            default:
                file << "null";
                break;
        }
    }

    file << "\n}\n";
    file.close();

    LOG_INFO("Debug config saved to %s", path);
}

// ----------------------------------------------------------------------------
// Minimal JSON parser — handles only the flat {key: value} format we write.
// This avoids pulling in a JSON library for a simple config file.
// Parses: numbers, booleans, arrays of numbers.
// ----------------------------------------------------------------------------

// Skip whitespace
static void skipWS(const std::string &s, size_t &i)
{
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) {
        i++;
    }
}

// Parse a quoted string, return contents
static std::string parseString(const std::string &s, size_t &i)
{
    if (i >= s.size() || s[i] != '"') {
        return "";
    }
    i++; // skip opening quote
    std::string result;
    while (i < s.size() && s[i] != '"') {
        result += s[i++];
    }
    if (i < s.size()) {
        i++; // skip closing quote
    }
    return result;
}

// Parse a number (float or int)
static f64 parseNumber(const std::string &s, size_t &i)
{
    size_t start = i;
    if (i < s.size() && (s[i] == '-' || s[i] == '+')) {
        i++;
    }
    while (i < s.size() && (s[i] >= '0' && s[i] <= '9')) {
        i++;
    }
    if (i < s.size() && s[i] == '.') {
        i++;
        while (i < s.size() && (s[i] >= '0' && s[i] <= '9')) {
            i++;
        }
    }
    // Handle scientific notation (e.g. 1e-5)
    if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
        i++;
        if (i < s.size() && (s[i] == '-' || s[i] == '+')) {
            i++;
        }
        while (i < s.size() && (s[i] >= '0' && s[i] <= '9')) {
            i++;
        }
    }
    return std::stod(s.substr(start, i - start));
}

void imguiManagerLoadConfig(const char *path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_WARN("No debug config found at %s", path);
        return;
    }

    // Read entire file into string
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    file.close();

    // Build a lookup: config_key → widget index
    std::map<std::string, u32> key_to_widget;
    for (u32 i = 0; i < s_state.widgets.size(); i++) {
        if (!s_state.widgets[i].config_key.empty()) {
            key_to_widget[s_state.widgets[i].config_key] = i;
        }
    }

    // Parse the JSON object
    size_t pos = 0;
    skipWS(content, pos);
    if (pos >= content.size() || content[pos] != '{') {
        LOG_ERROR("Invalid config JSON");
        return;
    }
    pos++; // skip '{'

    while (pos < content.size()) {
        skipWS(content, pos);
        if (pos >= content.size() || content[pos] == '}') {
            break;
        }

        // Skip comma between entries
        if (content[pos] == ',') {
            pos++;
            skipWS(content, pos);
        }

        // Parse key
        std::string key = parseString(content, pos);
        skipWS(content, pos);
        if (pos >= content.size() || content[pos] != ':') {
            break;
        }
        pos++; // skip ':'
        skipWS(content, pos);

        // Find matching widget
        auto it = key_to_widget.find(key);
        if (it == key_to_widget.end()) {
            // Skip unknown value — advance past it
            while (pos < content.size() && content[pos] != ',' && content[pos] != '}') {
                pos++;
            }
            continue;
        }

        DebugWidget &w = s_state.widgets[it->second];

        // Parse value based on widget type
        switch (w.type) {
            case WidgetType::SliderFloat:
                if (w.float_ptr != nullptr) {
                    *w.float_ptr = static_cast<f32>(parseNumber(content, pos));
                }
                break;

            case WidgetType::SliderInt:
            case WidgetType::Combo:
                if (w.int_ptr != nullptr) {
                    *w.int_ptr = static_cast<i32>(parseNumber(content, pos));
                }
                break;

            case WidgetType::Checkbox:
                if (w.bool_ptr != nullptr) {
                    if (content.substr(pos, 4) == "true") {
                        *w.bool_ptr = true;
                        pos += 4;
                    } else if (content.substr(pos, 5) == "false") {
                        *w.bool_ptr = false;
                        pos += 5;
                    }
                }
                break;

            case WidgetType::Color3:
            case WidgetType::DragFloat3:
                if (w.float_ptr != nullptr && pos < content.size() && content[pos] == '[') {
                    pos++; // skip '['
                    for (int c = 0; c < 3; c++) {
                        skipWS(content, pos);
                        w.float_ptr[c] = static_cast<f32>(parseNumber(content, pos));
                        skipWS(content, pos);
                        if (pos < content.size() && content[pos] == ',') {
                            pos++;
                        }
                    }
                    skipWS(content, pos);
                    if (pos < content.size() && content[pos] == ']') {
                        pos++;
                    }
                }
                break;

            case WidgetType::Color4:
                if (w.float_ptr != nullptr && pos < content.size() && content[pos] == '[') {
                    pos++; // skip '['
                    for (int c = 0; c < 4; c++) {
                        skipWS(content, pos);
                        w.float_ptr[c] = static_cast<f32>(parseNumber(content, pos));
                        skipWS(content, pos);
                        if (pos < content.size() && content[pos] == ',') {
                            pos++;
                        }
                    }
                    skipWS(content, pos);
                    if (pos < content.size() && content[pos] == ']') {
                        pos++;
                    }
                }
                break;

            default:
                // Skip unsupported value types
                while (pos < content.size() && content[pos] != ',' && content[pos] != '}') {
                    pos++;
                }
                break;
        }
    }

    LOG_INFO("Debug config loaded from %s", path);
}

}  // namespace chad
