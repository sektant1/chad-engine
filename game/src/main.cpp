// ============================================================================
// Chad Engine — Dungeon Demo
//
// PS1-style FPS dungeon crawler demo.
// Procedurally generates a dungeon, renders with PSX shaders,
// and lets you walk around with full Fauerby collision.
// ============================================================================

#include <engine/core/types.h>
#include <engine/core/log.h>
#include <engine/core/math.h>
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

#include <glad/glad.h>
#include <cstdio>

using namespace chad;

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

    // ---- Renderer (PSX internal resolution) ----
    RendererConfig render_cfg {};
    render_cfg.internal_width  = 640;
    render_cfg.internal_height = 480;
    Renderer *renderer         = rendererCreate(render_cfg);

    rendererSetClearColor(0.02F, 0.02F, 0.05F, 1.0F);

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

    // ---- Materials ----
    Material mat_floor   = materialCreate(&psx_shader, &tex_floor);
    Material mat_ceiling = materialCreate(&psx_shader, &tex_ceiling);
    Material mat_wall    = materialCreate(&psx_shader, &tex_wall);

    // ---- Generate dungeon ----
    DungeonConfig dung_cfg {};
    dung_cfg.grid_width  = 48;
    dung_cfg.grid_height = 48;
    dung_cfg.room_count  = 10;
    dung_cfg.wall_height = 4.0F;
    dung_cfg.cell_size   = 3.0F;
    dung_cfg.seed        = 0;  // random each run

    Dungeon dungeon = dungeonGenerate(dung_cfg);

    // ---- Collision world ----
    CollisionWorld col_world;
    col_world.addTriangles(dungeon.collision);

    // ---- Player controller ----
    PlayerController player;
    Vec3             spawn = dungeonGetSpawnPosition(dungeon);
    player.position        = {spawn.x, 1.0F, spawn.z};
    player.radius          = 0.3F;
    player.stand_height    = 0.8F;
    player.move_speed      = 6.0F;
    player.gravity         = -18.0F;
    player.jump_force      = 6.5F;

    // ---- Camera ----
    Camera cam {};
    cam.fov        = 90.0F;
    cam.near_plane = 0.1F;
    cam.far_plane  = 60.0F;

    // ---- PSX shader defaults ----
    shaderBind(psx_shader);
    shaderSetFloat(psx_shader, "uSnapResolution", 160.0F);
    shaderSetFloat(psx_shader, "uFogStart", 5.0F);
    shaderSetFloat(psx_shader, "uFogEnd", 40.0F);
    shaderSetVec3(psx_shader, "uFogColor", 0.02F, 0.02F, 0.05F);
    shaderSetInt(psx_shader, "uDitheringEnabled", 1);
    shaderSetFloat(psx_shader, "uColorDepth", 31.0F);
    shaderSetVec4(psx_shader, "uTintColor", 1.0F, 1.0F, 1.0F, 1.0F);
    shaderUnbind();

    LOG_INFO("Demo started. WASD=move, Mouse=look, Space=jump, Ctrl=crouch, Esc=quit");
    LOG_INFO("Spawn at (%.1f, %.1f, %.1f)", spawn.x, spawn.y, spawn.z);

    // ================================================================
    // Main loop
    // ================================================================

    while (!windowShouldClose(window)) {
        inputUpdate();
        timerUpdate();

        f32 dt = static_cast<f32>(timerGetDelta());
        if (dt > 0.05F) {
            dt = 0.05F;  // clamp to avoid physics explosions
        }

        // ---- Input ----
        if (inputKeyPressed(Key::Escape)) {
            break;
        }

        // Toggle cursor lock with Tab
        static bool cursor_locked = true;
        if (inputKeyPressed(Key::Tab)) {
            cursor_locked = !cursor_locked;
            inputSetCursorLocked(cursor_locked);
        }

        // Mouse look
        if (cursor_locked) {
            cameraProcessMouse(cam, inputMouseDX(), inputMouseDY());
        }

        // Movement input (relative to camera facing)
        Vec3 move_input = {0, 0, 0};
        Vec3 cam_fwd    = cameraForward(cam);
        Vec3 cam_right  = cameraRight(cam);

        // Project to XZ plane for FPS movement
        Vec3 fwd_flat   = vec3Normalize({cam_fwd.x, 0, cam_fwd.z});
        Vec3 right_flat = vec3Normalize({cam_right.x, 0, cam_right.z});

        if (inputKeyDown(Key::W)) {
            move_input += fwd_flat;
        }
        if (inputKeyDown(Key::S)) {
            move_input -= fwd_flat;
        }
        if (inputKeyDown(Key::D)) {
            move_input += right_flat;
        }
        if (inputKeyDown(Key::A)) {
            move_input -= right_flat;
        }

        // Normalize diagonal movement
        if (vec3LenSquare(move_input) > 1.0F) {
            move_input = vec3Normalize(move_input);
        }

        bool jump   = inputKeyPressed(Key::Space);
        bool crouch = inputKeyDown(Key::LeftCtrl);

        // ---- Update player ----
        player.update(col_world, dt, move_input, jump, crouch);

        // ---- Sync camera to player eye ----
        cam.position = player.getEyePosition();

        // ---- Render ----
        i32 win_w = 0;
        i32 win_h = 0;
        windowGetFramebufferSize(window, &win_w, &win_h);
        f32 aspect = (win_h > 0) ? static_cast<f32>(win_w) / static_cast<f32>(win_h) : 1.0F;

        Matrix4 view  = cameraViewMatrix(cam);
        Matrix4 proj  = cameraProjectionMatrix(cam, aspect);
        Matrix4 model = matIdentity();

        // Begin rendering to FBO at PSX resolution
        rendererBeginFrame(renderer);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        // Set shared uniforms
        shaderBind(psx_shader);
        shaderSetMatrix4(psx_shader, "uView", view);
        shaderSetMatrix4(psx_shader, "uProjection", proj);
        shaderSetMatrix4(psx_shader, "uModel", model);

        // Draw floor
        materialBind(mat_floor);
        meshDraw(dungeon.mesh.floor_mesh);

        // Draw ceiling
        materialBind(mat_ceiling);
        meshDraw(dungeon.mesh.ceiling_mesh);

        // Draw walls
        materialBind(mat_wall);
        meshDraw(dungeon.mesh.wall_mesh);

        materialUnbind();
        shaderUnbind();

        rendererEndFrame(renderer);

        // Blit FBO to screen (nearest-neighbor upscale = pixelated PSX look)
        rendererPresent(renderer, win_w, win_h);

        windowSwapBuffers(window);
    }

    // ================================================================
    // Cleanup
    // ================================================================

    dungeonDestroyMesh(dungeon);
    textureDestroy(tex_floor);
    textureDestroy(tex_ceiling);
    textureDestroy(tex_wall);
    shaderDestroy(psx_shader);
    rendererDestroy(renderer);
    windowDestroy(window);

    return 0;
}
