// ============================================================================
// Dungeon Props — Implementation
//
// Prop placement strategy:
//   1. Feature-driven: LOOT→chest, PILLAR→pillar, TORCH→candle
//   2. Room decoration: corners get barrels/boxes, walls get skull mounts
//   3. Corridor decoration: chains, debris scattered randomly
//   4. Room-center: large rooms get chandeliers or arches
//
// All placement uses the same xorshift32 RNG as dungeon generation,
// seeded from the dungeon seed, so props are deterministic.
// ============================================================================

#include "dungeon/dungeon_props.h"
#include <engine/core/log.h>

#include <ctime>

namespace chad
{

// ============================================================================
// Prop definitions table — the single source of truth for all prop types.
// To add a new prop: add enum value, add row here, done.
// ============================================================================

// clang-format off
// Scales computed from actual FBX vertex bounds → target world size:
//   Barrel:     8u wide  → want ~0.7  → 0.09
//   Box:        8u wide  → want ~0.6  → 0.075
//   Chest:     12u deep  → want ~0.8  → 0.07
//   Pillar:    30u tall  → want ~4.0  → 0.13
//   Candle:     3u tall  → want ~0.5  → 0.15
//   Chain:     10u tall  → want ~2.0  → 0.20
//   Chandelier:20u wide  → want ~1.5  → 0.08
//   Skull_Wall:20u wide  → want ~1.0  → 0.05
//   Spikes:    18u wide  → want ~1.0  → 0.07
//   Debris:    11u wide  → want ~0.6  → 0.05
//   Door_Frame:20u wide  → want ~3.0  → 0.15
//   Arch:      20u wide  → want ~3.0  → 0.15
const PropDef PROP_DEFS[] = {
    // type              model_path                                              texture_path                                            scale    collide wall    col_radius
    {PropType::BARREL,     "assets/models/PSX_Dungeon/Models/Barrel.fbx",         "assets/models/PSX_Dungeon/Textures/TEX_Barrel_01.png",   0.09F,   true,   false,  0.35F},
    {PropType::BOX,        "assets/models/PSX_Dungeon/Models/Box.fbx",            "assets/models/PSX_Dungeon/Textures/TEX_Crate_01.png",    0.075F,  true,   false,  0.30F},
    {PropType::CHEST,      "assets/models/PSX_Dungeon/Models/Chest.fbx",          "assets/models/PSX_Dungeon/Textures/TEX_Planks_01.png",   0.07F,   true,   false,  0.40F},
    {PropType::PILLAR,     "assets/models/PSX_Dungeon/Models/Pillar.fbx",         "assets/models/PSX_Dungeon/Textures/TEX_Pillar_01.png",   0.13F,   true,   false,  0.35F},
    {PropType::CANDLE,     "assets/models/PSX_Dungeon/Models/Candle_01.fbx",      "assets/models/PSX_Dungeon/Textures/TEX_Candle_01.png",   0.15F,   false,  false,  0.0F},
    {PropType::CHAIN,      "assets/models/PSX_Dungeon/Models/Chain.fbx",          "assets/models/PSX_Dungeon/Textures/TEX_Chain_02.png",    0.20F,   false,  false,  0.0F},
    {PropType::CHANDELIER, "assets/models/PSX_Dungeon/Models/Chandelier.fbx",     "assets/models/PSX_Dungeon/Textures/TEX_Metal_01.png",    0.08F,   false,  false,  0.0F},
    {PropType::SKULL_WALL, "assets/models/PSX_Dungeon/Models/Skull_Wall.fbx",     "assets/models/PSX_Dungeon/Textures/TEX_Skull_Wall.png",  0.05F,   false,  true,   0.0F},
    {PropType::SPIKES,     "assets/models/PSX_Dungeon/Models/Spikes.fbx",         "assets/models/PSX_Dungeon/Textures/TEX_Metal_01.png",    0.07F,   false,  false,  0.0F},
    {PropType::DEBRIS,     "assets/models/PSX_Dungeon/Models/Debris.fbx",         "assets/models/PSX_Dungeon/Textures/TEX_Wall_03.png",     0.05F,   false,  false,  0.0F},
    {PropType::DOOR_FRAME, "assets/models/PSX_Dungeon/Models/Door_Frame_01.fbx",  "assets/models/PSX_Dungeon/Textures/TEX_Door_01.png",     0.15F,   false,  true,   0.0F},
    {PropType::ARCH,       "assets/models/PSX_Dungeon/Models/Arch.fbx",           "assets/models/PSX_Dungeon/Textures/TEX_Arch_02.png",     0.15F,   false,  false,  0.0F},
};
// clang-format on

const u32 PROP_DEF_COUNT = sizeof(PROP_DEFS) / sizeof(PROP_DEFS[0]);

// ============================================================================
// RNG — same xorshift32 as dungeon_map.cpp for deterministic placement.
// ============================================================================

struct PropRng
{
    u32 state;

    explicit PropRng(u32 seed)
        : state(seed != 0 ? seed : static_cast<u32>(time(nullptr)))
    {
    }

    u32 next()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    i32 range(i32 lo, i32 hi)
    {
        if (lo >= hi) {
            return lo;
        }
        return lo + static_cast<i32>(next() % static_cast<u32>(hi - lo));
    }

    i32 percent() { return range(0, 100); }

    // Random float in [0, 1)
    f32 frand() { return static_cast<f32>(next() % 10000) / 10000.0F; }
};

// ============================================================================
// Model cache
// ============================================================================

void propCacheInit(PropModelCache &cache)
{
    for (u32 i = 0; i < (u32)PropType::COUNT; i++) {
        cache.loaded[i] = false;
    }

    for (u32 i = 0; i < PROP_DEF_COUNT; i++) {
        const PropDef &def = PROP_DEFS[i];
        u32 idx = (u32)def.type;

        cache.models[idx] = modelLoad(def.model_path);
        cache.loaded[idx] = (cache.models[idx].mesh_count > 0);

        // Load manual texture override — PSX_Dungeon FBX files often don't
        // embed textures, so we load them separately and bind before drawing.
        if (def.texture_path != nullptr) {
            cache.textures[idx] = textureLoad(def.texture_path);
        }

        if (!cache.loaded[idx]) {
            LOG_WARN("Failed to load prop model: %s", def.model_path);
        }
    }

    LOG_INFO("Prop cache initialized: %u types", PROP_DEF_COUNT);
}

void propCacheDestroy(PropModelCache &cache)
{
    for (u32 i = 0; i < (u32)PropType::COUNT; i++) {
        if (cache.loaded[i]) {
            modelDestroy(cache.models[i]);
            if (cache.textures[i].id != 0) {
                textureDestroy(cache.textures[i]);
            }
            cache.loaded[i] = false;
        }
    }
}

// ============================================================================
// Prop placement
//
// Strategy:
//   - Feature-based: directly map dungeon features → prop types
//   - Room corners: scatter barrels/boxes in room corners
//   - Corridors: occasional debris/chains
//   - Large rooms: chandelier in center ceiling
// ============================================================================

// Find wall direction for wall-mounted props.
// Returns rotation in degrees (0=+Z, 90=+X, 180=-Z, 270=-X) or -1 if no wall.
static f32 findWallDirection(const Dungeon &dungeon, i32 gx, i32 gz)
{
    if (dungeon.cellAt(gx, gz - 1) == CellType::SOLID) {
        return 0.0F;    // north wall → face south (+Z)
    }
    if (dungeon.cellAt(gx + 1, gz) == CellType::SOLID) {
        return 90.0F;   // east wall → face west
    }
    if (dungeon.cellAt(gx, gz + 1) == CellType::SOLID) {
        return 180.0F;  // south wall → face north
    }
    if (dungeon.cellAt(gx - 1, gz) == CellType::SOLID) {
        return 270.0F;  // west wall → face east
    }
    return -1.0F;
}

// Offset position toward a wall based on direction.
static Vec3 wallOffset(Vec3 pos, f32 dir_deg, f32 amount)
{
    if (dir_deg < 45.0F) {
        return {pos.x, pos.y, pos.z - amount}; // toward north wall
    }
    if (dir_deg < 135.0F) {
        return {pos.x + amount, pos.y, pos.z}; // toward east wall
    }
    if (dir_deg < 225.0F) {
        return {pos.x, pos.y, pos.z + amount}; // toward south wall
    }
    return {pos.x - amount, pos.y, pos.z};     // toward west wall
}

void dungeonPlaceProps(const Dungeon &dungeon, std::vector<PropInstance> &out_props, u32 seed)
{
    out_props.clear();

    // Use a different seed offset so props don't correlate with room placement.
    PropRng rng(seed + 0xDEAD);

    const f32 cs = dungeon.config.cell_size;
    const f32 wh = dungeon.config.wall_height;

    // --- Feature-driven props ---
    for (const Feature &feat : dungeon.features) {
        Vec3 world_pos = dungeon.gridToWorldCenter(feat.x, feat.z);

        switch (feat.type) {
            case FeatureType::LOOT: {
                // Chest at loot position
                PropInstance p;
                p.type       = PropType::CHEST;
                p.position   = world_pos;
                p.rotation_y = static_cast<f32>(rng.range(0, 4)) * 90.0F;
                p.scale      = PROP_DEFS[(u32)PropType::CHEST].default_scale;
                out_props.push_back(p);
                break;
            }

            case FeatureType::PILLAR: {
                // Pillar at pillar position
                PropInstance p;
                p.type       = PropType::PILLAR;
                p.position   = world_pos;
                p.rotation_y = 0.0F;
                p.scale      = PROP_DEFS[(u32)PropType::PILLAR].default_scale;
                out_props.push_back(p);
                break;
            }

            case FeatureType::TORCH: {
                // Candle near wall
                f32 wall_dir = findWallDirection(dungeon, feat.x, feat.z);
                if (wall_dir >= 0.0F) {
                    PropInstance p;
                    p.type       = PropType::CANDLE;
                    p.position   = wallOffset(world_pos, wall_dir, cs * 0.35F);
                    p.position.y = dungeon.config.floor_y;
                    p.rotation_y = wall_dir;
                    p.scale      = PROP_DEFS[(u32)PropType::CANDLE].default_scale;
                    out_props.push_back(p);
                }
                break;
            }

            case FeatureType::TRAP: {
                // Spikes at trap position
                PropInstance p;
                p.type       = PropType::SPIKES;
                p.position   = world_pos;
                p.rotation_y = 0.0F;
                p.scale      = PROP_DEFS[(u32)PropType::SPIKES].default_scale;
                out_props.push_back(p);
                break;
            }

            default:
                break;
        }
    }

    // --- Room corner decoration (barrels, boxes) ---
    for (const Room &room : dungeon.rooms) {
        // Place barrels/boxes in 1-2 corners
        struct Corner { i32 x, z; };
        Corner corners[4] = {
            {room.x,              room.z},               // top-left
            {room.x + room.w - 1, room.z},               // top-right
            {room.x,              room.z + room.h - 1},   // bottom-left
            {room.x + room.w - 1, room.z + room.h - 1},  // bottom-right
        };

        i32 num_corner_props = rng.range(0, 3); // 0-2 corners get props
        for (i32 c = 0; c < num_corner_props; c++) {
            i32 ci = rng.range(0, 4);
            Vec3 corner_pos = dungeon.gridToWorldCenter(corners[ci].x, corners[ci].z);

            PropInstance p;
            p.type       = (rng.percent() < 60) ? PropType::BARREL : PropType::BOX;
            p.position   = corner_pos;
            p.rotation_y = static_cast<f32>(rng.range(0, 360));
            p.scale      = PROP_DEFS[(u32)p.type].default_scale;
            out_props.push_back(p);
        }

        // Large rooms (>=7x7) get a chandelier at ceiling height
        if (room.w >= 7 && room.h >= 7 && rng.percent() < 40) {
            Vec3 center = dungeon.gridToWorldCenter(room.center_x(), room.center_z());
            PropInstance p;
            p.type       = PropType::CHANDELIER;
            p.position   = {center.x, dungeon.config.floor_y + wh - 0.3F, center.z};
            p.rotation_y = 0.0F;
            p.scale      = PROP_DEFS[(u32)PropType::CHANDELIER].default_scale;
            out_props.push_back(p);
        }

        // Wall skull decoration — pick a random wall cell in the room
        if (rng.percent() < 30) {
            // Try north wall
            i32 sx = rng.range(room.x + 1, room.x + room.w - 1);
            i32 sz = room.z;
            f32 wall_dir = findWallDirection(dungeon, sx, sz);
            if (wall_dir >= 0.0F) {
                Vec3 pos = dungeon.gridToWorldCenter(sx, sz);
                PropInstance p;
                p.type       = PropType::SKULL_WALL;
                p.position   = wallOffset(pos, wall_dir, cs * 0.4F);
                p.position.y = dungeon.config.floor_y + wh * 0.6F;
                p.rotation_y = wall_dir + 180.0F; // face into room
                p.scale      = PROP_DEFS[(u32)PropType::SKULL_WALL].default_scale;
                out_props.push_back(p);
            }
        }
    }

    // --- Corridor decoration (debris, chains) ---
    for (i32 gz = 0; gz < dungeon.config.grid_height; gz++) {
        for (i32 gx = 0; gx < dungeon.config.grid_width; gx++) {
            if (dungeon.cellAt(gx, gz) != CellType::CORRIDOR) {
                continue;
            }

            // Low chance of debris in corridors
            if (rng.percent() < 3) {
                Vec3 pos = dungeon.gridToWorldCenter(gx, gz);
                PropInstance p;
                p.type       = PropType::DEBRIS;
                p.position   = pos;
                p.rotation_y = static_cast<f32>(rng.range(0, 360));
                p.scale      = PROP_DEFS[(u32)PropType::DEBRIS].default_scale;
                out_props.push_back(p);
            }

            // Chains hanging from corridor ceiling
            if (rng.percent() < 2) {
                Vec3 pos = dungeon.gridToWorldCenter(gx, gz);
                PropInstance p;
                p.type       = PropType::CHAIN;
                p.position   = {pos.x, dungeon.config.floor_y + wh - 0.1F, pos.z};
                p.rotation_y = 0.0F;
                p.scale      = PROP_DEFS[(u32)PropType::CHAIN].default_scale;
                out_props.push_back(p);
            }
        }
    }

    // --- Door frames at DOOR cells ---
    for (i32 gz = 0; gz < dungeon.config.grid_height; gz++) {
        for (i32 gx = 0; gx < dungeon.config.grid_width; gx++) {
            if (dungeon.cellAt(gx, gz) != CellType::DOOR) {
                continue;
            }

            Vec3 pos = dungeon.gridToWorldCenter(gx, gz);
            f32 wall_dir = findWallDirection(dungeon, gx, gz);

            PropInstance p;
            p.type       = PropType::DOOR_FRAME;
            p.position   = pos;
            p.rotation_y = (wall_dir >= 0.0F) ? wall_dir : 0.0F;
            p.scale      = PROP_DEFS[(u32)PropType::DOOR_FRAME].default_scale;
            out_props.push_back(p);
        }
    }

    LOG_INFO("Placed %u props", static_cast<u32>(out_props.size()));
}

// ============================================================================
// Prop rendering
// ============================================================================

Matrix4 propBuildMatrix(const PropInstance &prop)
{
    Matrix4 translate = matTranslate(prop.position);
    Matrix4 rotate    = matRotateY(toRadians(prop.rotation_y));
    Matrix4 scale     = matScale({prop.scale, prop.scale, prop.scale});
    return translate * rotate * scale;
}

void propsRender(const std::vector<PropInstance> &props,
                 const PropModelCache &cache,
                 const Shader &shader)
{
    for (const PropInstance &prop : props) {
        u32 idx = (u32)prop.type;
        if (!cache.loaded[idx]) {
            continue;
        }

        // Set model matrix for this prop
        Matrix4 model_mat = propBuildMatrix(prop);
        shaderSetMatrix4(shader, "uModel", model_mat);

        // Bind our manual texture override BEFORE drawing.
        // PSX_Dungeon FBX files don't embed textures — the model's own
        // meshes will have has_texture=false.  We bind our loaded texture
        // and draw each mesh manually to guarantee the texture is applied.
        const Model &model = cache.models[idx];
        bool has_override = (cache.textures[idx].id != 0);

        if (has_override) {
            textureBind(cache.textures[idx], 0);
            shaderSetInt(shader, "uUseTexture", 1);
        }

        for (u32 m = 0; m < model.mesh_count; m++) {
            const ModelMesh &mm = model.meshes[m];

            // If mesh has its own texture (embedded in GLB), use it.
            // Otherwise use our override texture.
            if (mm.has_texture) {
                textureBind(mm.texture, 0);
                shaderSetInt(shader, "uUseTexture", 1);
            } else if (!has_override) {
                // No texture at all — draw untextured (white)
                shaderSetInt(shader, "uUseTexture", 0);
            }

            meshDraw(mm.mesh);
        }
    }

    // Reset model matrix to identity after drawing all props
    Matrix4 identity = matIdentity();
    shaderSetMatrix4(shader, "uModel", identity);
    shaderSetInt(shader, "uUseTexture", 1);
}

// ============================================================================
// Prop collision — approximate each collidable prop with an AABB box.
//
// Generates 10 triangles (5 quads: 4 walls + floor) per prop.
// Skips ceiling to allow jumping on props.
// ============================================================================

void propsAddCollision(const std::vector<PropInstance> &props,
                       CollisionWorld &world)
{
    std::vector<Triangle> tris;

    for (const PropInstance &prop : props) {
        const PropDef &def = PROP_DEFS[(u32)prop.type];
        if (!def.has_collision) {
            continue;
        }

        f32 r = def.collision_radius;
        f32 h = 1.5F; // approximate prop height in world space

        Vec3 p = prop.position;
        f32 x0 = p.x - r, x1 = p.x + r;
        f32 z0 = p.z - r, z1 = p.z + r;
        f32 y0 = p.y,      y1 = p.y + h;

        // 4 wall faces (inward-facing normals for collision)
        // North
        tris.push_back({{x0, y0, z0}, {x1, y0, z0}, {x1, y1, z0}, {0, 0, 1}});
        tris.push_back({{x0, y0, z0}, {x1, y1, z0}, {x0, y1, z0}, {0, 0, 1}});
        // South
        tris.push_back({{x1, y0, z1}, {x0, y0, z1}, {x0, y1, z1}, {0, 0, -1}});
        tris.push_back({{x1, y0, z1}, {x0, y1, z1}, {x1, y1, z1}, {0, 0, -1}});
        // West
        tris.push_back({{x0, y0, z1}, {x0, y0, z0}, {x0, y1, z0}, {1, 0, 0}});
        tris.push_back({{x0, y0, z1}, {x0, y1, z0}, {x0, y1, z1}, {1, 0, 0}});
        // East
        tris.push_back({{x1, y0, z0}, {x1, y0, z1}, {x1, y1, z1}, {-1, 0, 0}});
        tris.push_back({{x1, y0, z0}, {x1, y1, z1}, {x1, y1, z0}, {-1, 0, 0}});
        // Top (walk on top of prop)
        tris.push_back({{x0, y1, z0}, {x1, y1, z0}, {x1, y1, z1}, {0, 1, 0}});
        tris.push_back({{x0, y1, z0}, {x1, y1, z1}, {x0, y1, z1}, {0, 1, 0}});
    }

    if (!tris.empty()) {
        world.addTriangles(tris);
        LOG_INFO("Added %u collision tris from %u props",
                 static_cast<u32>(tris.size()), static_cast<u32>(props.size()));
    }
}

}  // namespace chad
