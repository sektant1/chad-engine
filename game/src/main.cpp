// ============================================================================
// Chad Engine — Dungeon Demo
//
// PS1-style FPS dungeon crawler with multi-floor dungeons, props, and portals.
//
// Controls:
//   WASD    — move       Space  — jump       Ctrl — crouch
//   Mouse   — look       Tab    — cursor     Esc  — quit
//   F1      — debug UI   F2     — input capture toggle
//
// Dungeon world:
//   3 procedural floors, connected by portals at EXIT features.
//   Props from PSX_Dungeon assets placed automatically.
//   Walk into the glowing exit area to descend to the next floor.
// ============================================================================

#include <engine/core/types.h>
#include <engine/core/log.h>
#include <engine/core/math.h>
#include <engine/core/imgui_manager.h>
#include <engine/platform/window.h>
#include <engine/platform/input.h>
#include <engine/platform/timer.h>
#include <engine/renderer/renderer.h>
#include <engine/renderer/shader.h>
#include <engine/renderer/mesh.h>
#include <engine/renderer/texture.h>
#include <engine/renderer/material.h>
#include <engine/renderer/camera.h>
#include <engine/physics/collision.h>

#include "dungeon/dungeon_map.h"
#include "dungeon/dungeon_props.h"
#include "dungeon/dungeon_world.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdio>

using namespace chad;

// ============================================================================
// PSX state — all retro shader tunables.
// ============================================================================
struct PSXState
{
    f32  snap_resolution   = 160.0F;
    f32  fog_start         = 5.0F;
    f32  fog_end           = 40.0F;
    f32  fog_color[3]      = {0.02F, 0.02F, 0.05F};
    f32  clear_color[3]    = {0.02F, 0.02F, 0.05F};
    bool dithering_enabled = true;
    f32  color_depth       = 31.0F;
    f32  tint_color[4]     = {1.0F, 1.0F, 1.0F, 1.0F};
};

// ============================================================================
// Register debug widgets.
// ============================================================================
static void registerDebugWidgets(PSXState &psx, Camera &cam, Renderer *renderer,
                                  PlayerController &player)
{
    static i32 res_w = 320;
    static i32 res_h = 240;
    rendererGetInternalSize(renderer, &res_w, &res_h);

    imguiManagerAddSliderInt("Resolution", "Width",  &res_w, 160, 1920, "res_width");
    imguiManagerAddSliderInt("Resolution", "Height", &res_h, 120, 1080, "res_height");
    imguiManagerAddButton("Resolution", "Apply Resolution", [renderer]() {
        rendererResize(renderer, res_w, res_h);
    });

    imguiManagerAddSliderFloat("Camera", "FOV",         &cam.fov,           60.0F, 120.0F, "%.0f", "cam_fov");
    imguiManagerAddSliderFloat("Camera", "Sensitivity",  &cam.sensitivity,  0.05F, 0.50F, "%.3f", "cam_sensitivity");
    imguiManagerAddSliderFloat("Camera", "Move Speed",   &player.move_speed, 1.0F, 20.0F, "%.1f", "move_speed");
    imguiManagerAddSliderFloat("Camera", "Jump Force",   &player.jump_force, 1.0F, 15.0F, "%.1f", "jump_force");

    imguiManagerAddSliderFloat("Vertex Snapping", "Snap Resolution", &psx.snap_resolution,
                                0.0F, 320.0F, "%.0f", "snap_resolution");
    imguiManagerAddText("Vertex Snapping", "", "0=off, 80=strong jitter, 160=subtle, 320=barely visible");

    imguiManagerAddColor3("Fog", "Fog Color",  psx.fog_color,  "fog_color");
    imguiManagerAddSliderFloat("Fog", "Fog Start", &psx.fog_start, 0.0F, 50.0F, "%.1f", "fog_start");
    imguiManagerAddSliderFloat("Fog", "Fog End",   &psx.fog_end,   1.0F, 100.0F, "%.1f", "fog_end");

    imguiManagerAddColor3("Color & Post", "Clear Color", psx.clear_color, "clear_color");
    imguiManagerAddColor4("Color & Post", "Tint",        psx.tint_color,  "tint_color");
    imguiManagerAddCheckbox("Color & Post", "Dithering",  &psx.dithering_enabled, "dithering");
    imguiManagerAddSliderFloat("Color & Post", "Color Depth", &psx.color_depth,
                                3.0F, 255.0F, "%.0f", "color_depth");
    imguiManagerAddText("Color & Post", "", "31=PS1 (15-bit), 63=18-bit, 255=full 24-bit");
}

// ============================================================================
// Push PSX uniforms to shader.
// ============================================================================
static void pushPSXUniforms(const Shader &shader, const PSXState &psx)
{
    shaderSetFloat(shader, "uSnapResolution", psx.snap_resolution);
    shaderSetFloat(shader, "uFogStart", psx.fog_start);
    shaderSetFloat(shader, "uFogEnd", psx.fog_end);
    shaderSetVec3(shader, "uFogColor", psx.fog_color[0], psx.fog_color[1], psx.fog_color[2]);
    shaderSetInt(shader, "uDitheringEnabled", psx.dithering_enabled ? 1 : 0);
    shaderSetFloat(shader, "uColorDepth", psx.color_depth);
    shaderSetVec4(shader, "uTintColor", psx.tint_color[0], psx.tint_color[1],
                  psx.tint_color[2], psx.tint_color[3]);
}

// ============================================================================
// Entry point
// ============================================================================
int main()
{
    // ---- Window ----
    WindowConfig win_cfg {};
    win_cfg.title  = "Chad Engine — Dungeon Demo";
    win_cfg.vsync  = true;
    Window *window = windowCreate(win_cfg);

    inputInit(window);
    inputSetCursorLocked(true);
    timerInit();

    // ---- Renderer ----
    RendererConfig render_cfg {};
    render_cfg.internal_width  = 320;
    render_cfg.internal_height = 240;
    Renderer *renderer         = rendererCreate(render_cfg);

    PSXState psx {};
    rendererSetClearColor(psx.clear_color[0], psx.clear_color[1], psx.clear_color[2], 1.0F);

    // ---- ImGui ----
    GLFWwindow *glfw_win = static_cast<GLFWwindow *>(windowGetNativeHandle(window));
    imguiManagerInit(glfw_win);

    // ---- Shaders ----
    Shader psx_shader = shaderLoad("assets/shaders/psx.vert", "assets/shaders/psx.frag");
    if (psx_shader.program == 0) {
        LOG_ERROR("Failed to load PSX shader!");
        return 1;
    }

    // ---- Textures ----
    Texture tex_floor   = textureLoad("assets/textures/Cobble.png");
    Texture tex_ceiling = textureLoad("assets/textures/Cobble_Ceiling.png");
    Texture tex_wall    = textureLoad("assets/textures/Cobble_Wall.png");

    // ---- Load prop models ----
    PropModelCache prop_cache {};
    propCacheInit(prop_cache);

    // ---- Generate multi-floor dungeon ----
    DungeonConfig base_cfg {};
    base_cfg.grid_width  = 24;
    base_cfg.grid_height = 24;
    base_cfg.room_count  = 6;
    base_cfg.wall_height = 4.0F;
    base_cfg.cell_size   = 3.0F;
    base_cfg.seed        = 0;  // random each run

    FloorTextures floor_tex;
    floor_tex.shader  = &psx_shader;
    floor_tex.floor   = &tex_floor;
    floor_tex.ceiling = &tex_ceiling;
    floor_tex.wall    = &tex_wall;

    constexpr i32 NUM_FLOORS = 3;
    DungeonWorld world = dungeonWorldCreate(NUM_FLOORS, base_cfg, floor_tex);

    // ---- Player controller ----
    PlayerController player;
    Vec3 spawn     = dungeonWorldGetSpawnPosition(world);
    player.position     = {spawn.x, 1.0F, spawn.z};
    player.radius       = 0.3F;
    player.stand_height = 0.8F;
    player.move_speed   = 6.0F;
    player.gravity      = -18.0F;
    player.jump_force   = 6.5F;

    // ---- Camera ----
    Camera cam {};
    cam.fov        = 90.0F;
    cam.near_plane = 0.1F;
    cam.far_plane  = 60.0F;

    // ---- Debug widgets ----
    registerDebugWidgets(psx, cam, renderer, player);
    imguiManagerLoadConfig();

    // Push initial uniforms
    shaderBind(psx_shader);
    pushPSXUniforms(psx_shader, psx);
    shaderUnbind();

    LOG_INFO("Demo started. %d floors. WASD=move, F1=debug, Esc=quit", NUM_FLOORS);
    LOG_INFO("Walk to the EXIT area to descend to the next floor!");
    LOG_INFO("Spawn at (%.1f, %.1f, %.1f)", spawn.x, spawn.y, spawn.z);

    // ================================================================
    // Main loop
    // ================================================================

    bool cursor_locked = true;

    // Cooldown prevents re-triggering portal immediately after transition
    f32 portal_cooldown = 0.0F;

    while (!windowShouldClose(window)) {
        inputUpdate();
        timerUpdate();

        f32 dt = static_cast<f32>(timerGetDelta());
        if (dt > 0.05F) {
            dt = 0.05F;
        }

        // ---- Portal cooldown ----
        if (portal_cooldown > 0.0F) {
            portal_cooldown -= dt;
        }

        // ---- Debug UI toggles ----
        if (inputKeyPressed(Key::F1)) {
            imguiManagerToggleWindow();
            if (imguiManagerIsVisible()) {
                cursor_locked = false;
                inputSetCursorLocked(false);
            } else {
                cursor_locked = true;
                inputSetCursorLocked(true);
            }
        }
        if (inputKeyPressed(Key::F2)) {
            imguiManagerToggleInputCapture();
        }

        // ---- Game Input ----
        bool game_has_input = !imguiManagerWantsInput();

        if (inputKeyPressed(Key::Escape)) {
            break;
        }

        if (game_has_input && inputKeyPressed(Key::Tab)) {
            cursor_locked = !cursor_locked;
            inputSetCursorLocked(cursor_locked);
        }

        if (cursor_locked && game_has_input) {
            cameraProcessMouse(cam, inputMouseDX(), inputMouseDY());
        }

        Vec3 move_input = {0, 0, 0};
        bool jump       = false;
        bool crouch     = false;

        if (game_has_input) {
            Vec3 cam_fwd    = cameraForward(cam);
            Vec3 cam_right  = cameraRight(cam);
            Vec3 fwd_flat   = vec3Normalize({cam_fwd.x, 0, cam_fwd.z});
            Vec3 right_flat = vec3Normalize({cam_right.x, 0, cam_right.z});

            if (inputKeyDown(Key::W)) { move_input += fwd_flat; }
            if (inputKeyDown(Key::S)) { move_input -= fwd_flat; }
            if (inputKeyDown(Key::D)) { move_input += right_flat; }
            if (inputKeyDown(Key::A)) { move_input -= right_flat; }

            if (vec3LenSquare(move_input) > 1.0F) {
                move_input = vec3Normalize(move_input);
            }

            jump   = inputKeyPressed(Key::Space);
            crouch = inputKeyDown(Key::LeftCtrl);
        }

        // ---- Update player against active floor's collision ----
        CollisionWorld &col_world = dungeonWorldGetCollision(world);
        player.update(col_world, dt, move_input, jump, crouch);

        // ---- Check portal transitions ----
        if (portal_cooldown <= 0.0F) {
            i32 target = dungeonWorldCheckPortals(world, player.position, player.radius);
            if (target >= 0) {
                dungeonWorldTransition(world, target, player);
                portal_cooldown = 2.0F; // 2s cooldown prevents bounce
            }
        }

        // ---- Sync camera ----
        cam.position = player.getEyePosition();

        // ---- Update clear color ----
        rendererSetClearColor(psx.clear_color[0], psx.clear_color[1], psx.clear_color[2], 1.0F);

        // ---- Render ----
        i32 win_w = 0, win_h = 0;
        windowGetFramebufferSize(window, &win_w, &win_h);
        f32 aspect = (win_h > 0) ? static_cast<f32>(win_w) / static_cast<f32>(win_h) : 1.0F;

        Matrix4 view  = cameraViewMatrix(cam);
        Matrix4 proj  = cameraProjectionMatrix(cam, aspect);
        Matrix4 model = matIdentity();

        rendererBeginFrame(renderer);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        shaderBind(psx_shader);
        pushPSXUniforms(psx_shader, psx);
        shaderSetMatrix4(psx_shader, "uView", view);
        shaderSetMatrix4(psx_shader, "uProjection", proj);
        shaderSetMatrix4(psx_shader, "uModel", model);

        // Draw active floor geometry (floor, ceiling, walls)
        dungeonWorldRenderFloor(world);

        // Draw props on active floor
        // Props use same PSX shader — model matrix set per-prop inside propsRender
        dungeonWorldRenderProps(world, prop_cache, psx_shader);

        // Reset model matrix after props
        shaderSetMatrix4(psx_shader, "uModel", model);

        materialUnbind();
        shaderUnbind();

        rendererEndFrame(renderer);
        rendererPresent(renderer, win_w, win_h);

        // ---- ImGui ----
        imguiManagerNewFrame();

        i32 int_w = 0, int_h = 0;
        rendererGetInternalSize(renderer, &int_w, &int_h);
        OverlayStats overlay;
        overlay.internal_w  = int_w;
        overlay.internal_h  = int_h;
        overlay.snap_res    = psx.snap_resolution;
        overlay.color_depth = psx.color_depth;
        overlay.dithering   = psx.dithering_enabled;
        overlay.fog_start   = psx.fog_start;
        overlay.fog_end     = psx.fog_end;
        imguiManagerSetOverlayStats(overlay);

        imguiManagerDraw(&cam, renderer);
        imguiManagerRender();

        windowSwapBuffers(window);
    }

    // ================================================================
    // Cleanup
    // ================================================================

    imguiManagerShutdown();
    dungeonWorldDestroy(world);
    propCacheDestroy(prop_cache);
    textureDestroy(tex_floor);
    textureDestroy(tex_ceiling);
    textureDestroy(tex_wall);
    shaderDestroy(psx_shader);
    rendererDestroy(renderer);
    windowDestroy(window);

    return 0;
}
