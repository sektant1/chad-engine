#pragma once

// ============================================================================
// Procedural Dungeon Generator — Rooms + Corridors + Feature Placer
//
// 2D-grid-based generation → 3D triangle mesh output.
// Designed for PS1-style FPS dungeon crawlers with Fauerby collision.
//
// Pipeline:
//   1. Fill grid with SOLID
//   2. Place rooms (random size/position, no overlap)
//   3. Connect rooms with L-shaped corridors
//   4. Place features (spawn, exit, torches, traps, loot) in rooms
//   5. Generate 3D mesh: floor, ceiling, walls with per-face texcoords
//   6. Export collision triangles for CollisionWorld
// ============================================================================

#include <engine/core/types.h>
#include <engine/core/math.h>
#include <engine/renderer/mesh.h>
#include <engine/physics/collision.h>

#include <vector>
#include <cstdlib>

namespace chad
{

// ---------------------------------------------------------------------------
// Grid cell types
// ---------------------------------------------------------------------------

enum class CellType : u8
{
    SOLID    = 0,
    FLOOR    = 1,  // room interior
    CORRIDOR = 2,  // hallway
    DOOR     = 3,  // doorway between room and corridor
};

// ---------------------------------------------------------------------------
// Feature types placed in rooms
// ---------------------------------------------------------------------------

enum class FeatureType : u8
{
    NONE,
    SPAWN,   // player start
    EXIT,    // stairs down / level exit
    TORCH,   // wall light
    TRAP,    // floor trap trigger
    LOOT,    // treasure chest
    ENEMY,   // enemy spawn point
    PILLAR,  // decorative column (solid)
};

struct Feature
{
    FeatureType type = FeatureType::NONE;
    i32         x    = 0;
    i32         z    = 0;
};

// ---------------------------------------------------------------------------
// Room descriptor
// ---------------------------------------------------------------------------

struct Room
{
    i32 x, z;  // top-left corner in grid coords
    i32 w, h;  // width (X), height (Z)

    i32 center_x() const { return x + w / 2; }

    i32 center_z() const { return z + h / 2; }
};

// ---------------------------------------------------------------------------
// Generation config — all the knobs
// ---------------------------------------------------------------------------

struct DungeonConfig
{
    // Grid
    i32 grid_width  = 48;
    i32 grid_height = 48;

    // Rooms
    i32 room_min_size = 4;
    i32 room_max_size = 10;
    i32 room_count    = 12;  // attempts; actual count may be less
    i32 room_padding  = 1;   // min gap between rooms

    // Corridors
    i32 corridor_width = 2;  // 1 = narrow, 2 = comfortable

    // 3D geometry
    f32 cell_size   = 3.0F;  // world units per grid cell
    f32 wall_height = 4.0F;  // floor to ceiling
    f32 floor_y     = 0.0F;  // Y of the floor plane

    // Features
    i32 torches_per_room = 2;
    i32 trap_chance      = 15;  // percent per eligible room
    i32 loot_chance      = 20;
    i32 enemy_chance     = 40;
    i32 pillar_chance    = 10;

    // Seed
    u32 seed = 0;  // 0 = random
};

// ---------------------------------------------------------------------------
// Generated dungeon output
// ---------------------------------------------------------------------------

struct DungeonMesh
{
    // Separate meshes for different textures.
    Mesh floor_mesh;
    Mesh ceiling_mesh;
    Mesh wall_mesh;
    u32  floor_tri_count;
    u32  ceiling_tri_count;
    u32  wall_tri_count;
};

struct Dungeon
{
    DungeonConfig         config;
    std::vector<CellType> grid;  // grid_width * grid_height
    std::vector<Room>     rooms;
    std::vector<Feature>  features;
    DungeonMesh           mesh;
    std::vector<Triangle> collision;  // for CollisionWorld

    // Grid accessors
    CellType cellAt(i32 x, i32 z) const;
    void     setCell(i32 x, i32 z, CellType type);
    bool     inBounds(i32 x, i32 z) const;

    // World-space conversion
    Vec3 gridToWorld(i32 gx, i32 gz) const;
    Vec3 gridToWorldCenter(i32 gx, i32 gz) const;
};

// ---------------------------------------------------------------------------
// Generation API
// ---------------------------------------------------------------------------

// Generate a complete dungeon from config.
Dungeon dungeonGenerate(const DungeonConfig &config);

// Build 3D mesh + collision triangles from the grid.
// Called automatically by dungeonGenerate, but can be called again
// after modifying the grid.
void dungeonBuildMesh(Dungeon &dungeon);

// Free GPU resources.
void dungeonDestroyMesh(Dungeon &dungeon);

// Get player spawn position in world space (center of spawn room, on floor).
Vec3 dungeonGetSpawnPosition(const Dungeon &dungeon);

// Get exit position.
Vec3 dungeonGetExitPosition(const Dungeon &dungeon);

// Get all features of a given type as world positions.
std::vector<Vec3> dungeonGetFeaturePositions(const Dungeon &dungeon, FeatureType type);

}  // namespace chad
