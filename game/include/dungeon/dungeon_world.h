#pragma once

// ============================================================================
// Dungeon World — Multi-floor dungeon system with portals
//
// Manages multiple dungeon floors, each with its own geometry, collision,
// and props.  Portals connect floors: stepping into a portal trigger
// transitions the player to the target floor's spawn point.
//
// Usage:
//   DungeonWorld world = dungeonWorldCreate(3, base_config, textures);
//   // ... game loop ...
//   i32 portal = dungeonWorldCheckPortals(world, player_pos, 1.0F);
//   if (portal >= 0) dungeonWorldTransition(world, portal, player);
//   // ... render ...
//   dungeonWorldRenderFloor(world, shader);
//   dungeonWorldRenderProps(world, prop_cache, shader);
// ============================================================================

#include <engine/core/types.h>
#include <engine/core/math.h>
#include <engine/renderer/shader.h>
#include <engine/renderer/texture.h>
#include <engine/renderer/material.h>
#include <engine/physics/collision.h>

#include "dungeon_map.h"
#include "dungeon_props.h"

#include <vector>

namespace chad
{

struct PlayerController;

// ----------------------------------------------------------------------------
// Portal — connects two dungeon floors.
//
// Placed at EXIT features.  Each floor's exit leads to the next floor's
// spawn, except the last floor which loops back to floor 0.
// ----------------------------------------------------------------------------
struct Portal
{
    Vec3 position;         // world-space center of portal trigger
    f32  radius = 1.5F;    // trigger radius
    i32  source_floor;     // floor this portal is on
    i32  target_floor;     // floor it leads to
    Vec3 target_position;  // where player arrives on target floor
};

// ----------------------------------------------------------------------------
// Dungeon floor — one level of the dungeon.
//
// Contains everything needed to render and simulate one floor:
// geometry, collision, props, and its own collision world.
// ----------------------------------------------------------------------------
struct DungeonFloor
{
    Dungeon                    dungeon;
    CollisionWorld             collision;
    std::vector<PropInstance>  props;
    i32                        floor_index = 0;

    // Materials for this floor (shared textures, floor owns the Material structs)
    Material mat_floor;
    Material mat_ceiling;
    Material mat_wall;
};

// ----------------------------------------------------------------------------
// Floor textures — passed to dungeonWorldCreate so each floor can
// build its materials.  Textures are shared (not duplicated per floor).
// ----------------------------------------------------------------------------
struct FloorTextures
{
    Shader  *shader;
    Texture *floor;
    Texture *ceiling;
    Texture *wall;
};

// ----------------------------------------------------------------------------
// Dungeon World — the top-level container.
// ----------------------------------------------------------------------------
struct DungeonWorld
{
    std::vector<DungeonFloor> floors;
    std::vector<Portal>       portals;
    i32                       active_floor = 0;
    i32                       num_floors   = 0;
};

// ============================================================================
// API
// ============================================================================

// Create a multi-floor dungeon world.
// Each floor gets progressively harder (more rooms, more enemies).
// base_config is used as template — room_count, enemy_chance, etc.
// scale up per floor.
DungeonWorld dungeonWorldCreate(i32 num_floors,
                                const DungeonConfig &base_config,
                                const FloorTextures &textures);

// Destroy all floors' GPU resources.
void dungeonWorldDestroy(DungeonWorld &world);

// Get active floor reference.
DungeonFloor &dungeonWorldActiveFloor(DungeonWorld &world);
const DungeonFloor &dungeonWorldActiveFloor(const DungeonWorld &world);

// Check if player is inside any portal on the active floor.
// Returns target_floor index, or -1 if not in any portal.
i32 dungeonWorldCheckPortals(const DungeonWorld &world, Vec3 player_pos, f32 player_radius);

// Transition to a new floor.  Moves the player to that floor's spawn.
void dungeonWorldTransition(DungeonWorld &world, i32 target_floor,
                            PlayerController &player);

// Get spawn position for the active floor.
Vec3 dungeonWorldGetSpawnPosition(const DungeonWorld &world);

// Render active floor geometry (floor, ceiling, walls).
void dungeonWorldRenderFloor(const DungeonWorld &world);

// Render active floor props.
void dungeonWorldRenderProps(const DungeonWorld &world,
                             const PropModelCache &cache,
                             const Shader &shader);

// Get collision world for active floor.
CollisionWorld &dungeonWorldGetCollision(DungeonWorld &world);

// Get the current floor number (0-based) for display.
i32 dungeonWorldCurrentFloor(const DungeonWorld &world);

// Total number of floors.
i32 dungeonWorldFloorCount(const DungeonWorld &world);

}  // namespace chad
