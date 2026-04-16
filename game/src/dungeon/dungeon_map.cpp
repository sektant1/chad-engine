// ============================================================================
// Procedural Dungeon Generator — Implementation
//
// Generation pipeline:
//   1. placeRooms()      — scatter non-overlapping rectangular rooms
//   2. connectRooms()    — L-shaped corridors between room centers
//   3. placeFeatures()   — spawn, exit, torches, traps, loot, enemies
//   4. dungeonBuildMesh() — grid → 3D triangle mesh + collision
// ============================================================================

#include "dungeon/dungeon_map.h"
#include <engine/core/log.h>
#include <engine/physics/math_ext.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <ctime>

namespace chad
{

// ============================================================================
// RNG helper — simple xorshift32 (deterministic, fast)
// ============================================================================

struct Rng
{
    u32 state;

    explicit Rng(u32 seed)
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
};

// ============================================================================
// Dungeon grid accessors
// ============================================================================

CellType Dungeon::cellAt(i32 x, i32 z) const
{
    if (!inBounds(x, z)) {
        return CellType::SOLID;
    }
    return grid[static_cast<usize>((z * config.grid_width) + x)];
}

void Dungeon::setCell(i32 x, i32 z, CellType type)
{
    if (!inBounds(x, z)) {
        return;
    }
    grid[static_cast<usize>((z * config.grid_width) + x)] = type;
}

bool Dungeon::inBounds(i32 x, i32 z) const
{
    return x >= 0 && x < config.grid_width && z >= 0 && z < config.grid_height;
}

Vec3 Dungeon::gridToWorld(i32 gx, i32 gz) const
{
    return {static_cast<f32>(gx) * config.cell_size, config.floor_y, static_cast<f32>(gz) * config.cell_size};
}

Vec3 Dungeon::gridToWorldCenter(i32 gx, i32 gz) const
{
    f32 half = config.cell_size * 0.5F;
    return {
        static_cast<f32>(gx) * config.cell_size + half, config.floor_y, static_cast<f32>(gz) * config.cell_size + half};
}

// ============================================================================
// Step 1: Place rooms
// ============================================================================

static bool roomsOverlap(const Room &a, const Room &b, i32 pad)
{
    return !(a.x + a.w + pad <= b.x || b.x + b.w + pad <= a.x || a.z + a.h + pad <= b.z || b.z + b.h + pad <= a.z);
}

static void placeRooms(Dungeon &dungeon, Rng &rng)
{
    const auto &cfg = dungeon.config;

    for (i32 attempt = 0; attempt < cfg.room_count * 10; ++attempt) {
        if (static_cast<i32>(dungeon.rooms.size()) >= cfg.room_count) {
            break;
        }

        Room room {};
        room.w = rng.range(cfg.room_min_size, cfg.room_max_size + 1);
        room.h = rng.range(cfg.room_min_size, cfg.room_max_size + 1);
        room.x = rng.range(2, cfg.grid_width - room.w - 2);
        room.z = rng.range(2, cfg.grid_height - room.h - 2);

        bool overlaps = false;
        for (const Room &existing : dungeon.rooms) {
            if (roomsOverlap(room, existing, cfg.room_padding)) {
                overlaps = true;
                break;
            }
        }
        if (overlaps) {
            continue;
        }

        // Carve room into grid.
        for (i32 z = room.z; z < room.z + room.h; ++z) {
            for (i32 x = room.x; x < room.x + room.w; ++x) {
                dungeon.setCell(x, z, CellType::FLOOR);
            }
        }

        dungeon.rooms.push_back(room);
    }
}

// ============================================================================
// Step 2: Connect rooms with L-shaped corridors
// ============================================================================

static void carveCorridor(Dungeon &dungeon, i32 x1, i32 z1, i32 x2, i32 z2, i32 width)
{
    // Horizontal segment first, then vertical.
    i32 dx = (x2 > x1) ? 1 : -1;
    i32 dz = (z2 > z1) ? 1 : -1;

    // Horizontal leg
    for (i32 x = x1; x != x2 + dx; x += dx) {
        for (i32 w = 0; w < width; ++w) {
            i32 zz = z1 + w - width / 2;
            if (dungeon.cellAt(x, zz) == CellType::SOLID) {
                dungeon.setCell(x, zz, CellType::CORRIDOR);
            }
        }
    }

    // Vertical leg
    for (i32 z = z1; z != z2 + dz; z += dz) {
        for (i32 w = 0; w < width; ++w) {
            i32 xx = x2 + w - width / 2;
            if (dungeon.cellAt(xx, z) == CellType::SOLID) {
                dungeon.setCell(xx, z, CellType::CORRIDOR);
            }
        }
    }
}

static void connectRooms(Dungeon &dungeon, Rng &rng)
{
    if (dungeon.rooms.size() < 2) {
        return;
    }

    // Sort rooms by distance to first room for a spanning-tree-ish layout.
    // Simple approach: connect each room to the next in the sorted list.
    std::vector<usize> order(dungeon.rooms.size());
    for (usize i = 0; i < order.size(); ++i) {
        order[i] = i;
    }

    // Shuffle pairs slightly for variety.
    for (usize i = 1; i < order.size(); ++i) {
        usize j = static_cast<usize>(rng.range(0, static_cast<i32>(i + 1)));
        if (i != j) {
            usize tmp = order[i];
            order[i]  = order[j];
            order[j]  = tmp;
        }
    }

    for (usize i = 0; i + 1 < order.size(); ++i) {
        const Room &a = dungeon.rooms[order[i]];
        const Room &b = dungeon.rooms[order[i + 1]];
        carveCorridor(dungeon, a.center_x(), a.center_z(), b.center_x(), b.center_z(), dungeon.config.corridor_width);
    }

    // Extra random connections for loops (makes exploration more interesting).
    i32 extras = rng.range(1, static_cast<i32>(dungeon.rooms.size()) / 3 + 1);
    for (i32 e = 0; e < extras; ++e) {
        i32 ia = rng.range(0, static_cast<i32>(dungeon.rooms.size()));
        i32 ib = rng.range(0, static_cast<i32>(dungeon.rooms.size()));
        if (ia != ib) {
            carveCorridor(dungeon,
                          dungeon.rooms[static_cast<usize>(ia)].center_x(),
                          dungeon.rooms[static_cast<usize>(ia)].center_z(),
                          dungeon.rooms[static_cast<usize>(ib)].center_x(),
                          dungeon.rooms[static_cast<usize>(ib)].center_z(),
                          dungeon.config.corridor_width);
        }
    }
}

// ============================================================================
// Step 3: Place features
// ============================================================================

static void placeFeatures(Dungeon &dungeon, Rng &rng)
{
    if (dungeon.rooms.empty()) {
        return;
    }

    // Spawn in first room.
    {
        const Room &r = dungeon.rooms[0];
        dungeon.features.push_back({FeatureType::SPAWN, r.center_x(), r.center_z()});
    }

    // Exit in last room.
    {
        const Room &r = dungeon.rooms.back();
        dungeon.features.push_back({FeatureType::EXIT, r.center_x(), r.center_z()});
    }

    // Per-room features (skip first and last).
    for (usize i = 1; i + 1 < dungeon.rooms.size(); ++i) {
        const Room &r   = dungeon.rooms[i];
        const auto &cfg = dungeon.config;

        // Torches on walls.
        for (i32 t = 0; t < cfg.torches_per_room; ++t) {
            // Place on north or south wall edge.
            i32 tx = rng.range(r.x + 1, r.x + r.w - 1);
            i32 tz = (t % 2 == 0) ? r.z : r.z + r.h - 1;
            dungeon.features.push_back({FeatureType::TORCH, tx, tz});
        }

        if (rng.percent() < cfg.trap_chance) {
            dungeon.features.push_back({FeatureType::TRAP, r.center_x(), r.center_z()});
        }
        if (rng.percent() < cfg.loot_chance) {
            i32 lx = rng.range(r.x + 1, r.x + r.w - 1);
            i32 lz = rng.range(r.z + 1, r.z + r.h - 1);
            dungeon.features.push_back({FeatureType::LOOT, lx, lz});
        }
        if (rng.percent() < cfg.enemy_chance) {
            dungeon.features.push_back({FeatureType::ENEMY, r.center_x() + 1, r.center_z()});
        }
        if (rng.percent() < cfg.pillar_chance && r.w >= 6 && r.h >= 6) {
            dungeon.features.push_back({FeatureType::PILLAR, r.center_x(), r.center_z()});
        }
    }
}

// ============================================================================
// Step 4: Build 3D mesh + collision
//
// For each open cell, emit:
//   - Floor quad (2 tris)
//   - Ceiling quad (2 tris)
//   - Wall quads for each neighboring SOLID cell (2 tris per wall)
//
// Normals point inward (toward the open space).
// Texcoords tile at 1:1 ratio with cell_size.
// ============================================================================

struct MeshBuilder
{
    std::vector<Vertex> vertices;
    std::vector<u32>    indices;

    void addQuad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 normal, f32 u0, f32 v0, f32 u1, f32 v1)
    {
        u32 base = static_cast<u32>(vertices.size());

        auto makeVert = [&](Vec3 pos, f32 u, f32 v)
        {
            Vertex vert {};
            vert.position[0] = pos.x;
            vert.position[1] = pos.y;
            vert.position[2] = pos.z;
            vert.texcoord[0] = u;
            vert.texcoord[1] = v;
            vert.normal[0]   = normal.x;
            vert.normal[1]   = normal.y;
            vert.normal[2]   = normal.z;
            vert.color[0]    = 1.0F;
            vert.color[1]    = 1.0F;
            vert.color[2]    = 1.0F;
            vert.color[3]    = 1.0F;
            return vert;
        };

        // Quad: a-b-c, a-c-d (CCW)
        vertices.push_back(makeVert(a, u0, v0));
        vertices.push_back(makeVert(b, u1, v0));
        vertices.push_back(makeVert(c, u1, v1));
        vertices.push_back(makeVert(d, u0, v1));

        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    Mesh build()
    {
        if (vertices.empty()) {
            return {};
        }
        return meshCreate(
            vertices.data(), static_cast<u32>(vertices.size()), indices.data(), static_cast<u32>(indices.size()));
    }

    u32 triCount() const { return static_cast<u32>(indices.size()) / 3; }
};

static void addCollisionQuad(std::vector<Triangle> &out, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 normal)
{
    out.push_back({a, b, c, normal});
    out.push_back({a, c, d, normal});
}

void dungeonBuildMesh(Dungeon &dungeon)
{
    MeshBuilder floors;
    MeshBuilder ceilings;
    MeshBuilder walls;

    const f32 cs      = dungeon.config.cell_size;
    const f32 floor_y = dungeon.config.floor_y;
    const f32 ceil_y  = floor_y + dungeon.config.wall_height;
    const f32 wh      = dungeon.config.wall_height;

    dungeon.collision.clear();

    for (i32 gz = 0; gz < dungeon.config.grid_height; ++gz) {
        for (i32 gx = 0; gx < dungeon.config.grid_width; ++gx) {
            CellType cell = dungeon.cellAt(gx, gz);
            if (cell == CellType::SOLID) {
                continue;
            }

            f32 x0 = static_cast<f32>(gx) * cs;
            f32 z0 = static_cast<f32>(gz) * cs;
            f32 x1 = x0 + cs;
            f32 z1 = z0 + cs;

            // Floor (normal up)
            Vec3 fa      = {x0, floor_y, z1};
            Vec3 fb      = {x1, floor_y, z1};
            Vec3 fc      = {x1, floor_y, z0};
            Vec3 fd      = {x0, floor_y, z0};
            Vec3 floor_n = {0, 1, 0};
            floors.addQuad(fa, fb, fc, fd, floor_n, 0, 0, 1, 1);
            addCollisionQuad(dungeon.collision, fa, fb, fc, fd, floor_n);

            // Ceiling (normal down)
            Vec3 ca     = {x0, ceil_y, z0};
            Vec3 cb     = {x1, ceil_y, z0};
            Vec3 cc     = {x1, ceil_y, z1};
            Vec3 cd     = {x0, ceil_y, z1};
            Vec3 ceil_n = {0, -1, 0};
            ceilings.addQuad(ca, cb, cc, cd, ceil_n, 0, 0, 1, 1);
            addCollisionQuad(dungeon.collision, ca, cb, cc, cd, ceil_n);

            // Walls: check each neighbor. If neighbor is SOLID, emit a wall.
            // Wall texcoords: U = horizontal span, V = vertical (0=floor, 1=ceiling)
            f32 uv_h = wh / cs;  // V scale for wall height

            // North wall (neighbor at gz-1 is solid → wall faces +Z)
            if (dungeon.cellAt(gx, gz - 1) == CellType::SOLID) {
                Vec3 wa = {x0, floor_y, z0};
                Vec3 wb = {x1, floor_y, z0};
                Vec3 wc = {x1, ceil_y, z0};
                Vec3 wd = {x0, ceil_y, z0};
                Vec3 wn = {0, 0, 1};
                walls.addQuad(wa, wb, wc, wd, wn, 0, 0, 1, uv_h);
                addCollisionQuad(dungeon.collision, wa, wb, wc, wd, wn);
            }

            // South wall (neighbor at gz+1 is solid → wall faces -Z)
            if (dungeon.cellAt(gx, gz + 1) == CellType::SOLID) {
                Vec3 wa = {x1, floor_y, z1};
                Vec3 wb = {x0, floor_y, z1};
                Vec3 wc = {x0, ceil_y, z1};
                Vec3 wd = {x1, ceil_y, z1};
                Vec3 wn = {0, 0, -1};
                walls.addQuad(wa, wb, wc, wd, wn, 0, 0, 1, uv_h);
                addCollisionQuad(dungeon.collision, wa, wb, wc, wd, wn);
            }

            // West wall (neighbor at gx-1 is solid → wall faces +X)
            if (dungeon.cellAt(gx - 1, gz) == CellType::SOLID) {
                Vec3 wa = {x0, floor_y, z1};
                Vec3 wb = {x0, floor_y, z0};
                Vec3 wc = {x0, ceil_y, z0};
                Vec3 wd = {x0, ceil_y, z1};
                Vec3 wn = {1, 0, 0};
                walls.addQuad(wa, wb, wc, wd, wn, 0, 0, 1, uv_h);
                addCollisionQuad(dungeon.collision, wa, wb, wc, wd, wn);
            }

            // East wall (neighbor at gx+1 is solid → wall faces -X)
            if (dungeon.cellAt(gx + 1, gz) == CellType::SOLID) {
                Vec3 wa = {x1, floor_y, z0};
                Vec3 wb = {x1, floor_y, z1};
                Vec3 wc = {x1, ceil_y, z1};
                Vec3 wd = {x1, ceil_y, z0};
                Vec3 wn = {-1, 0, 0};
                walls.addQuad(wa, wb, wc, wd, wn, 0, 0, 1, uv_h);
                addCollisionQuad(dungeon.collision, wa, wb, wc, wd, wn);
            }
        }
    }

    dungeon.mesh.floor_mesh        = floors.build();
    dungeon.mesh.ceiling_mesh      = ceilings.build();
    dungeon.mesh.wall_mesh         = walls.build();
    dungeon.mesh.floor_tri_count   = floors.triCount();
    dungeon.mesh.ceiling_tri_count = ceilings.triCount();
    dungeon.mesh.wall_tri_count    = walls.triCount();

    LOG_INFO("Dungeon mesh: %u floor tris, %u ceiling tris, %u wall tris, %u collision tris",
             floors.triCount(),
             ceilings.triCount(),
             walls.triCount(),
             static_cast<u32>(dungeon.collision.size()));
}

// ============================================================================
// Destroy mesh GPU resources
// ============================================================================

void dungeonDestroyMesh(Dungeon &dungeon)
{
    if (dungeon.mesh.floor_mesh.vao != 0) {
        meshDestroy(dungeon.mesh.floor_mesh);
    }
    if (dungeon.mesh.ceiling_mesh.vao != 0) {
        meshDestroy(dungeon.mesh.ceiling_mesh);
    }
    if (dungeon.mesh.wall_mesh.vao != 0) {
        meshDestroy(dungeon.mesh.wall_mesh);
    }
    dungeon.collision.clear();
}

// ============================================================================
// Feature queries
// ============================================================================

Vec3 dungeonGetSpawnPosition(const Dungeon &dungeon)
{
    for (const Feature &f : dungeon.features) {
        if (f.type == FeatureType::SPAWN) {
            return dungeon.gridToWorldCenter(f.x, f.z);
        }
    }
    // Fallback: center of first room.
    if (!dungeon.rooms.empty()) {
        const Room &r = dungeon.rooms[0];
        return dungeon.gridToWorldCenter(r.center_x(), r.center_z());
    }
    return {0, 0, 0};
}

Vec3 dungeonGetExitPosition(const Dungeon &dungeon)
{
    for (const Feature &f : dungeon.features) {
        if (f.type == FeatureType::EXIT) {
            return dungeon.gridToWorldCenter(f.x, f.z);
        }
    }
    return {0, 0, 0};
}

std::vector<Vec3> dungeonGetFeaturePositions(const Dungeon &dungeon, FeatureType type)
{
    std::vector<Vec3> result;
    for (const Feature &f : dungeon.features) {
        if (f.type == type) {
            result.push_back(dungeon.gridToWorldCenter(f.x, f.z));
        }
    }
    return result;
}

// ============================================================================
// Main generation entry point
// ============================================================================

Dungeon dungeonGenerate(const DungeonConfig &config)
{
    Dungeon dungeon;
    dungeon.config = config;
    dungeon.grid.resize(static_cast<usize>(config.grid_width) * static_cast<usize>(config.grid_height),
                        CellType::SOLID);

    Rng rng(config.seed);

    placeRooms(dungeon, rng);
    connectRooms(dungeon, rng);
    placeFeatures(dungeon, rng);
    dungeonBuildMesh(dungeon);

    LOG_INFO("Dungeon generated: %u rooms, %u features, grid %dx%d",
             static_cast<u32>(dungeon.rooms.size()),
             static_cast<u32>(dungeon.features.size()),
             config.grid_width,
             config.grid_height);

    return dungeon;
}

}  // namespace chad
