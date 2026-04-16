#pragma once

// ============================================================================
// Dungeon Props — Data-driven prop placement using PSX_Dungeon assets
//
// Props are 3D models placed inside generated dungeons based on feature
// positions and room geometry.  The system is table-driven: PropDef defines
// what each prop type is (model path, scale, placement rules), and the
// placement function scatters PropInstances through the dungeon using
// deterministic RNG for reproducibility.
//
// Rendering:  Game loads all PropDef models once at startup, then each frame
//             iterates PropInstances and draws with a transform matrix built
//             from position/rotation/scale.
// Collision:  Props with has_collision=true get AABB collision boxes added
//             to the CollisionWorld (approximated, not per-triangle).
// ============================================================================

#include <engine/core/types.h>
#include <engine/core/math.h>
#include <engine/renderer/model.h>
#include <engine/renderer/shader.h>
#include <engine/renderer/texture.h>

#include "dungeon_map.h"

#include <vector>
#include <string>

namespace chad
{

// ----------------------------------------------------------------------------
// Prop type — each maps to a PSX_Dungeon model.
// Add new entries here + a row in PROP_DEFS[] to extend.
// ----------------------------------------------------------------------------
enum class PropType : u8
{
    BARREL,
    BOX,
    CHEST,
    PILLAR,
    CANDLE,
    CHAIN,
    CHANDELIER,
    SKULL_WALL,
    SPIKES,
    DEBRIS,
    DOOR_FRAME,
    ARCH,
    COUNT
};

// ----------------------------------------------------------------------------
// Prop definition — static table describing each prop type.
// ----------------------------------------------------------------------------
struct PropDef
{
    PropType    type;
    const char *model_path;       // path to FBX/GLB
    const char *texture_path;     // manual texture override (FBX may not embed)
    f32         default_scale;    // world-space scale factor
    bool        has_collision;    // add AABB collision box
    bool        wall_mounted;     // placed against walls vs free-standing on floor
    f32         collision_radius; // approximate AABB half-extent for collision
};

// Global prop definitions table — defined in .cpp
extern const PropDef PROP_DEFS[];
extern const u32     PROP_DEF_COUNT;

// ----------------------------------------------------------------------------
// Prop instance — one placed prop in a dungeon floor.
// ----------------------------------------------------------------------------
struct PropInstance
{
    PropType type;
    Vec3     position;
    f32      rotation_y = 0.0F;  // degrees around Y axis
    f32      scale      = 1.0F;
};

// ----------------------------------------------------------------------------
// Prop model cache — loaded once, shared across all dungeon floors.
// Index by (u32)PropType.
// ----------------------------------------------------------------------------
struct PropModelCache
{
    Model   models[(u32)PropType::COUNT];
    Texture textures[(u32)PropType::COUNT];  // manual texture overrides
    bool    loaded[(u32)PropType::COUNT];
};

// Load all prop models from disk.  Call once at startup.
void propCacheInit(PropModelCache &cache);

// Free all GPU resources.
void propCacheDestroy(PropModelCache &cache);

// ----------------------------------------------------------------------------
// Prop placement — scatter props through a generated dungeon.
//
// Uses dungeon features (LOOT→chest, PILLAR→pillar, TORCH→candle) and
// room geometry (corners→barrels, walls→skull_wall, corridors→chains)
// to place props deterministically from seed.
// ----------------------------------------------------------------------------
void dungeonPlaceProps(const Dungeon &dungeon, std::vector<PropInstance> &out_props, u32 seed);

// ----------------------------------------------------------------------------
// Prop rendering — draw all prop instances for one floor.
//
// Builds a model matrix per instance (translate * rotateY * scale) and
// draws with the given shader.  Shader must already have view/projection set.
// ----------------------------------------------------------------------------
void propsRender(const std::vector<PropInstance> &props,
                 const PropModelCache &cache,
                 const Shader &shader);

// Build transform matrix for a prop instance.
Matrix4 propBuildMatrix(const PropInstance &prop);

// Add prop collision AABBs to a collision world.
void propsAddCollision(const std::vector<PropInstance> &props,
                       CollisionWorld &world);

}  // namespace chad
