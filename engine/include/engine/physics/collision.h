#pragma once

// ============================================================================
// Collision system — Fauerby ellipsoid collide-and-slide
//
// Reference: Kasper Fauerby, "Improved Collision Detection and Response"
//            (2003, peroxide.dk)
//
// Architecture:
//   CollisionWorld   — owns static triangle mesh + trigger/liquid volumes
//   PlayerController — FPS ellipsoid controller built on collide-and-slide
//   Query helpers    — raycast, sphere sweep, overlap tests
// ============================================================================

#include <engine/core/types.h>
#include <engine/core/math.h>
#include <engine/physics/math_ext.h>

#include <vector>
#include <functional>

namespace chad
{

// ---------------------------------------------------------------------------
// Core geometry types
// ---------------------------------------------------------------------------

struct Triangle
{
    Vec3 a, b, c;
    Vec3 normal;  // precomputed face normal (must be unit length)
};

// Convenience: compute normal from vertices (CCW winding)
inline Triangle makeTriangle(Vec3 a, Vec3 b, Vec3 c)
{
    Triangle tri;
    tri.a      = a;
    tri.b      = b;
    tri.c      = c;
    tri.normal = vec3SafeNormalize(vec3Cross(b - a, c - a));
    return tri;
}

// ---------------------------------------------------------------------------
// Collision results
// ---------------------------------------------------------------------------

struct CollisionHit
{
    Vec3 point  = {0, 0, 0};
    Vec3 normal = {0, 1, 0};
    f32  t      = 1.0F;  // parametric time along sweep [0,1]
};

struct RaycastHit
{
    Vec3 point    = {0, 0, 0};
    Vec3 normal   = {0, 1, 0};
    f32  distance = 0.0F;
};

struct SweepHit
{
    Vec3 point  = {0, 0, 0};
    Vec3 normal = {0, 1, 0};
    f32  t      = 1.0F;
};

// ---------------------------------------------------------------------------
// CollisionPacket — Fauerby Section 2
//
// Carries all per-frame data for a single ellipsoid-vs-world sweep.
// Everything below the "eSpace" comment lives in *ellipsoid space* where
// the ellipsoid is a unit sphere.
// ---------------------------------------------------------------------------

struct CollisionPacket
{
    // Ellipsoid radii (half-extents) in world space.
    Vec3 e_radius = {0.3F, 0.9F, 0.3F};

    // ---- R3 (world) space ----
    Vec3 r3_position = {0, 0, 0};
    Vec3 r3_velocity = {0, 0, 0};

    // ---- eSpace (unit-sphere space) ----
    Vec3 base_point          = {0, 0, 0};
    Vec3 velocity            = {0, 0, 0};
    Vec3 normalized_velocity = {0, 0, 0};

    // ---- Collision output (eSpace) ----
    bool found_collision      = false;
    f32  nearest_distance     = 0.0F;
    Vec3 intersection_point   = {0, 0, 0};

    // ---- Response tracking ----
    Vec3 last_collision_normal = {0, 1, 0};  // world-space normal from last slide
};

// ---------------------------------------------------------------------------
// Trigger volumes — axis-aligned boxes and spheres for gameplay events
// ---------------------------------------------------------------------------

enum class TriggerShape : u8
{
    AABB,
    SPHERE
};

using TriggerCallback = std::function<void(u32 trigger_id)>;

struct TriggerVolume
{
    TriggerShape shape  = TriggerShape::AABB;
    Vec3         min    = {0, 0, 0};  // AABB min corner
    Vec3         max    = {0, 0, 0};  // AABB max corner
    Vec3         center = {0, 0, 0};  // sphere center
    f32          radius = 0.0F;       // sphere radius
    u32          id     = 0;
    bool         active = true;
    TriggerCallback on_enter = nullptr;
};

// ---------------------------------------------------------------------------
// Liquid volumes — horizontal water planes inside an AABB
// ---------------------------------------------------------------------------

struct LiquidVolume
{
    Vec3 min            = {0, 0, 0};
    Vec3 max            = {0, 0, 0};
    f32  surface_height = 0.0F;  // Y of water surface
    f32  buoyancy_force = 8.0F;  // upward acceleration when submerged
};

// ---------------------------------------------------------------------------
// Kinematic body — simplified capsule for enemies / props / doors
// ---------------------------------------------------------------------------

struct KinematicBody
{
    Vec3 position = {0, 0, 0};
    Vec3 velocity = {0, 0, 0};
    Vec3 e_radius = {0.3F, 0.9F, 0.3F};
    u32  id       = 0;
    bool active   = true;
};

// ---------------------------------------------------------------------------
// CollisionWorld
//
// Owns all static collision geometry and dynamic volumes.
// Performance note: currently brute-force over all triangles.
// For large meshes (> ~5k tris), replace the inner loops with a spatial
// structure — uniform grid, octree, or BVH.  For PS1-era poly counts
// (~500-2000 tris) brute-force is fine.
// ---------------------------------------------------------------------------

class CollisionWorld
{
public:
    // ---- Triangle mesh management ----
    void addTriangles(const Triangle *tris, u32 count);
    void addTriangles(const std::vector<Triangle> &tris);
    void clear();
    u32  triangleCount() const { return static_cast<u32>(m_triangles.size()); }
    const std::vector<Triangle> &triangles() const { return m_triangles; }

    // ---- Volume management ----
    void addTrigger(const TriggerVolume &trigger);
    void addLiquid(const LiquidVolume &liquid);
    void addKinematic(const KinematicBody &body);
    void clearVolumes();

    // ---- Core Fauerby collide-and-slide (public entry point) ----
    // Moves an ellipsoid from `position` along `velocity`, sliding on
    // collision.  Returns the final position.  `packet` is filled with
    // collision metadata (last normal, whether collision occurred, etc.).
    Vec3 collideAndSlide(CollisionPacket &packet, const Vec3 &position, const Vec3 &velocity);

    // ---- Query functions ----
    bool raycast(Vec3 origin, Vec3 dir, f32 max_dist, RaycastHit &out_hit) const;
    bool sphereSweep(Vec3 start, Vec3 end, f32 radius, SweepHit &out_hit) const;
    bool capsuleSweep(Vec3 start, Vec3 end, Vec3 e_radius, SweepHit &out_hit) const;
    bool overlapAABB(const Vec3 &box_min, const Vec3 &box_max) const;
    bool overlapSphere(Vec3 center, f32 radius) const;
    bool pointInLiquid(Vec3 pos, f32 &out_water_height) const;

    // ---- Trigger testing ----
    void checkTriggers(Vec3 position, f32 radius);

    // ---- Kinematic body update ----
    void updateKinematics(f32 dt);

    // ---- Debug draw (renders collision tris as wireframe GL_LINES) ----
    // Call once to build the debug mesh, then draw each frame.
    void debugBuild();
    void debugDraw(u32 debug_shader, const Matrix4 &view_proj) const;
    void debugDestroy();

private:
    // ---- Fauerby internals (eSpace) ----
    static constexpr i32 MAX_RECURSION_DEPTH = 5;

    void checkTriangle(CollisionPacket &packet, const Vec3 &p1, const Vec3 &p2, const Vec3 &p3) const;
    Vec3 collideWithWorld(CollisionPacket &packet, const Vec3 &pos, const Vec3 &vel, i32 depth) const;

    // ---- Ray-triangle (Moller-Trumbore) ----
    static bool rayTriangle(Vec3 origin, Vec3 dir, const Triangle &tri, f32 max_dist, f32 &out_t, Vec3 &out_point);

    // ---- Data ----
    std::vector<Triangle>      m_triangles;
    std::vector<TriggerVolume> m_triggers;
    std::vector<LiquidVolume>  m_liquids;
    std::vector<KinematicBody> m_kinematics;

    // Debug mesh (GL resources)
    u32 m_debug_vao = 0;
    u32 m_debug_vbo = 0;
    u32 m_debug_line_count = 0;
};

// ---------------------------------------------------------------------------
// PlayerController — polished FPS movement on top of CollisionWorld
//
// Uses an ellipsoid (radiusX = radiusZ, radiusY = half-height).
// Two-pass collide-and-slide: movement first, then gravity.
// This separation keeps horizontal wall-sliding independent of vertical
// slope/stair behaviour.
// ---------------------------------------------------------------------------

struct PlayerController
{
    // ---- Transform ----
    Vec3 position = {0.0F, 2.0F, 0.0F};
    Vec3 velocity = {0.0F, 0.0F, 0.0F};  // persistent (gravity accumulates here)

    // ---- Ellipsoid (capsule approximation) ----
    f32 radius        = 0.3F;   // XZ radius
    f32 stand_height  = 0.9F;   // Y half-height (standing)
    f32 crouch_height = 0.5F;   // Y half-height (crouched)

    // ---- Movement tuning ----
    f32 move_speed  = 5.0F;
    f32 jump_force  = 7.0F;
    f32 gravity     = -20.0F;
    f32 step_height = 0.3F;     // max stair step-up snap
    f32 coyote_time = 0.1F;     // seconds after leaving ground where jump still works

    // ---- Eye ----
    f32 eye_offset_stand  = 0.7F;   // from center of ellipsoid
    f32 eye_offset_crouch = 0.3F;

    // ---- State (read after Update) ----
    bool grounded            = false;
    bool in_water            = false;
    bool crouching           = false;
    f32  water_height        = 0.0F;
    Vec3 ground_normal       = {0.0F, 1.0F, 0.0F};
    f32  time_since_grounded = 1.0F;  // seconds since last on-ground frame

    // ---- API ----
    void update(CollisionWorld &world, f32 dt, Vec3 input_move, bool jump, bool crouch);
    Vec3 getPosition() const;
    Vec3 getEyePosition() const;

private:
    Vec3 currentERadius() const;
    void tryStepUp(CollisionWorld &world, const Vec3 &move_vel, f32 dt);
};

}  // namespace chad
