// ============================================================================
// Dungeon World — Multi-floor dungeon with portals
//
// Floor creation:
//   Each floor uses the base config as template with per-floor scaling:
//   - room_count increases by 2 per floor (more rooms deeper down)
//   - enemy_chance increases by 10% per floor (harder encounters)
//   - seed is derived from base seed + floor index (deterministic)
//
// Portal placement:
//   - Each floor's EXIT feature becomes a portal to the next floor's SPAWN
//   - Last floor's EXIT loops back to floor 0 (cycle / escape route)
//
// Transition:
//   - Player position is set to target floor's spawn
//   - Active collision world switches to target floor's
//   - No loading screen — floors are pre-generated at startup
// ============================================================================

#include "dungeon/dungeon_world.h"
#include <engine/core/log.h>
#include <engine/physics/collision.h>

namespace chad
{

DungeonWorld dungeonWorldCreate(i32 num_floors,
                                const DungeonConfig &base_config,
                                const FloorTextures &textures)
{
    DungeonWorld world;
    world.num_floors = num_floors;
    world.floors.resize(static_cast<usize>(num_floors));

    // --- Generate each floor ---
    for (i32 i = 0; i < num_floors; i++) {
        DungeonFloor &floor = world.floors[static_cast<usize>(i)];
        floor.floor_index   = i;

        // Scale config per floor — deeper = more rooms, harder enemies
        DungeonConfig cfg  = base_config;
        cfg.room_count     = base_config.room_count + i * 2;
        cfg.enemy_chance   = base_config.enemy_chance + i * 10;
        cfg.loot_chance    = base_config.loot_chance + i * 5;
        cfg.pillar_chance  = base_config.pillar_chance + i * 5;

        // Deterministic seed per floor: base_seed * 1000 + floor_index
        // If base seed is 0 (random), use a time-based offset per floor.
        if (base_config.seed != 0) {
            cfg.seed = base_config.seed * 1000 + static_cast<u32>(i);
        } else {
            cfg.seed = static_cast<u32>(i + 1) * 12345; // each floor unique
        }

        // Generate dungeon
        floor.dungeon = dungeonGenerate(cfg);

        // Build collision world
        floor.collision.addTriangles(floor.dungeon.collision);

        // Place props
        dungeonPlaceProps(floor.dungeon, floor.props, cfg.seed + 0xBEEF);

        // Add prop collision to the collision world
        propsAddCollision(floor.props, floor.collision);

        // Build materials (shared textures, per-floor material structs)
        floor.mat_floor   = materialCreate(textures.shader, textures.floor);
        floor.mat_ceiling = materialCreate(textures.shader, textures.ceiling);
        floor.mat_wall    = materialCreate(textures.shader, textures.wall);

        LOG_INFO("Floor %d: %u rooms, %u props, %u collision tris",
                 i,
                 static_cast<u32>(floor.dungeon.rooms.size()),
                 static_cast<u32>(floor.props.size()),
                 floor.collision.triangleCount());
    }

    // --- Create portals ---
    // Each floor's EXIT → next floor's SPAWN
    for (i32 i = 0; i < num_floors; i++) {
        i32 next = (i + 1) % num_floors; // last loops to 0

        Vec3 exit_pos  = dungeonGetExitPosition(world.floors[static_cast<usize>(i)].dungeon);
        Vec3 spawn_pos = dungeonGetSpawnPosition(world.floors[static_cast<usize>(next)].dungeon);

        Portal portal;
        portal.position        = exit_pos;
        portal.radius          = 1.5F;
        portal.source_floor    = i;
        portal.target_floor    = next;
        portal.target_position = spawn_pos;

        world.portals.push_back(portal);

        LOG_INFO("Portal: floor %d (%.1f, %.1f, %.1f) → floor %d (%.1f, %.1f, %.1f)",
                 i, exit_pos.x, exit_pos.y, exit_pos.z,
                 next, spawn_pos.x, spawn_pos.y, spawn_pos.z);
    }

    world.active_floor = 0;

    LOG_INFO("Dungeon world created: %d floors, %u portals",
             num_floors, static_cast<u32>(world.portals.size()));

    return world;
}

void dungeonWorldDestroy(DungeonWorld &world)
{
    for (auto &floor : world.floors) {
        dungeonDestroyMesh(floor.dungeon);
        floor.collision.clear();
    }
    world.floors.clear();
    world.portals.clear();
}

DungeonFloor &dungeonWorldActiveFloor(DungeonWorld &world)
{
    return world.floors[static_cast<usize>(world.active_floor)];
}

const DungeonFloor &dungeonWorldActiveFloor(const DungeonWorld &world)
{
    return world.floors[static_cast<usize>(world.active_floor)];
}

i32 dungeonWorldCheckPortals(const DungeonWorld &world, Vec3 player_pos, f32 player_radius)
{
    for (const Portal &portal : world.portals) {
        // Only check portals on the active floor
        if (portal.source_floor != world.active_floor) {
            continue;
        }

        // Simple sphere-sphere overlap test
        Vec3 diff = {
            player_pos.x - portal.position.x,
            0.0F,  // ignore Y for portal detection (floor-level)
            player_pos.z - portal.position.z
        };

        f32 dist_sq = diff.x * diff.x + diff.z * diff.z;
        f32 combined_radius = portal.radius + player_radius;

        if (dist_sq < combined_radius * combined_radius) {
            return portal.target_floor;
        }
    }

    return -1; // no portal hit
}

void dungeonWorldTransition(DungeonWorld &world, i32 target_floor,
                            PlayerController &player)
{
    if (target_floor < 0 || target_floor >= world.num_floors) {
        LOG_ERROR("Invalid floor transition: %d", target_floor);
        return;
    }

    i32 old_floor = world.active_floor;
    world.active_floor = target_floor;

    // Move player to target floor's spawn position
    Vec3 spawn = dungeonWorldGetSpawnPosition(world);
    player.position = {spawn.x, 1.0F, spawn.z};
    player.velocity = {0.0F, 0.0F, 0.0F};

    LOG_INFO("Transitioned: floor %d → floor %d, spawn (%.1f, %.1f, %.1f)",
             old_floor, target_floor, spawn.x, spawn.y, spawn.z);
}

Vec3 dungeonWorldGetSpawnPosition(const DungeonWorld &world)
{
    const DungeonFloor &floor = dungeonWorldActiveFloor(world);
    return dungeonGetSpawnPosition(floor.dungeon);
}

void dungeonWorldRenderFloor(const DungeonWorld &world)
{
    const DungeonFloor &floor = dungeonWorldActiveFloor(world);

    materialBind(floor.mat_floor);
    meshDraw(floor.dungeon.mesh.floor_mesh);

    materialBind(floor.mat_ceiling);
    meshDraw(floor.dungeon.mesh.ceiling_mesh);

    materialBind(floor.mat_wall);
    meshDraw(floor.dungeon.mesh.wall_mesh);

    materialUnbind();
}

void dungeonWorldRenderProps(const DungeonWorld &world,
                             const PropModelCache &cache,
                             const Shader &shader)
{
    const DungeonFloor &floor = dungeonWorldActiveFloor(world);
    propsRender(floor.props, cache, shader);
}

CollisionWorld &dungeonWorldGetCollision(DungeonWorld &world)
{
    return dungeonWorldActiveFloor(world).collision;
}

i32 dungeonWorldCurrentFloor(const DungeonWorld &world)
{
    return world.active_floor;
}

i32 dungeonWorldFloorCount(const DungeonWorld &world)
{
    return world.num_floors;
}

}  // namespace chad
