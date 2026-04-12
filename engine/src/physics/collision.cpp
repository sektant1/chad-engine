// ============================================================================
// Collision system implementation — Fauerby ellipsoid collide-and-slide
//
// This file implements:
//   1. CollisionWorld  — triangle storage, Fauerby core, query functions
//   2. PlayerController — FPS controller with two-pass slide + step-up
//   3. Debug draw       — wireframe triangle visualization
//
// The heart of the algorithm is checkTriangle() and collideWithWorld().
// Read the comments there for a step-by-step Fauerby walkthrough.
// ============================================================================

#include <engine/physics/collision.h>
#include <engine/core/log.h>

#include <glad/glad.h>

#include <algorithm>
#include <cmath>

namespace chad
{

// ============================================================================
// CollisionWorld — mesh management
// ============================================================================

void CollisionWorld::addTriangles(const Triangle *tris, u32 count)
{
    m_triangles.insert(m_triangles.end(), tris, tris + count);
}

void CollisionWorld::addTriangles(const std::vector<Triangle> &tris)
{
    m_triangles.insert(m_triangles.end(), tris.begin(), tris.end());
}

void CollisionWorld::clear()
{
    m_triangles.clear();
    clearVolumes();
}

void CollisionWorld::addTrigger(const TriggerVolume &trigger)
{
    m_triggers.push_back(trigger);
}

void CollisionWorld::addLiquid(const LiquidVolume &liquid)
{
    m_liquids.push_back(liquid);
}

void CollisionWorld::addKinematic(const KinematicBody &body)
{
    m_kinematics.push_back(body);
}

void CollisionWorld::clearVolumes()
{
    m_triggers.clear();
    m_liquids.clear();
    m_kinematics.clear();
}

// ============================================================================
// Fauerby core — checkTriangle()
//
// This is the innermost function of the collision system.  It tests whether
// a unit sphere (in eSpace) moving along `packet.velocity` from
// `packet.base_point` collides with a single triangle (p1, p2, p3).
//
// The algorithm proceeds in three stages:
//   A) Plane test — does the swept sphere touch the triangle's plane?
//   B) Interior test — is the contact point inside the triangle?
//   C) Vertex / edge test — if not, does the sphere graze a vertex or edge?
//
// All coordinates are in eSpace (ellipsoid space), where the ellipsoid is
// a unit sphere with radius 1.
// ============================================================================

void CollisionWorld::checkTriangle(CollisionPacket &packet,
                                   const Vec3 &p1,
                                   const Vec3 &p2,
                                   const Vec3 &p3) const
{
    // ---- Triangle plane ----
    Vec3 plane_normal = vec3SafeNormalize(vec3Cross(p2 - p1, p3 - p1));

    // Signed distance from the sphere center to the plane.
    // Positive = same side as normal.
    f32 signed_dist = vec3Dot(packet.base_point - p1, plane_normal);

    // How fast the sphere approaches the plane (negative = approaching).
    f32 normal_dot_vel = vec3Dot(plane_normal, packet.velocity);

    // ---- Stage A: determine t0, t1 (sphere touches/leaves plane) ----
    //
    // t0 = time the leading edge of the sphere first contacts the plane
    // t1 = time the trailing edge leaves the plane
    //
    // In eSpace the sphere has radius 1, so the leading edge is at
    //   base_point - plane_normal  (the point on the sphere closest to the plane)
    // The sphere center is at distance `signed_dist` from the plane.
    // Contact happens when that distance equals 1 (touching) or -1 (through).

    f32  t0             = 0.0F;
    f32  t1             = 1.0F;
    bool embedded       = false;

    if (fabsf(normal_dot_vel) < COLLISION_EPSILON) {
        // Moving (nearly) parallel to the plane.
        if (fabsf(signed_dist) >= 1.0F) {
            return;  // too far — no collision possible
        }
        // Sphere overlaps the plane for the entire sweep.
        // Can't collide with the plane *surface*, but might hit a vertex/edge.
        embedded = true;
        t0       = 0.0F;
        t1       = 1.0F;
    } else {
        // Standard case: two contact times.
        t0 = (-1.0F - signed_dist) / normal_dot_vel;
        t1 = (1.0F - signed_dist) / normal_dot_vel;

        if (t0 > t1) {
            f32 tmp = t0;
            t0      = t1;
            t1      = tmp;
        }

        // Range check against the sweep interval [0, 1].
        if (t0 > 1.0F || t1 < 0.0F) {
            return;
        }

        t0 = clampf(t0, 0.0F, 1.0F);
        t1 = clampf(t1, 0.0F, 1.0F);
    }

    // ---- Stage B: interior test ----
    //
    // At time t0 the sphere first touches the plane.  The contact point on
    // the plane is the sphere center at t0 *minus* the normal (because the
    // sphere's closest point to the plane is base - normal, and the center
    // has traveled t0 * velocity by then).

    bool found     = false;
    f32  t         = 1.0F;
    Vec3 col_point = {0, 0, 0};

    if (!embedded) {
        Vec3 plane_intersection = (packet.base_point - plane_normal) + (packet.velocity * t0);
        if (pointInTriangle(plane_intersection, p1, p2, p3)) {
            found     = true;
            t         = t0;
            col_point = plane_intersection;
        }
    }

    // ---- Stage C: vertex and edge sweep ----
    //
    // If the plane contact point misses the triangle interior (or we're
    // embedded), the sphere might still graze a vertex or edge.
    // We solve quadratic equations of the form:
    //
    //   |base + t*vel - vertex|^2 = 1   (vertex test)
    //   min_over_f |base + t*vel - (edge_start + f*edge)|^2 = 1  (edge test)
    //
    // We only need to check if we find a *closer* hit than what we already have.

    if (!found) {
        f32 vel_sq = vec3LenSquare(packet.velocity);

        // -- Vertex tests --
        // Check each of the three triangle vertices.
        const Vec3 *verts[3] = {&p1, &p2, &p3};
        for (int i = 0; i < 3; ++i) {
            Vec3 base_to_vert = packet.base_point - *verts[i];
            f32  a_coeff      = vel_sq;
            f32  b_coeff      = 2.0F * vec3Dot(packet.velocity, base_to_vert);
            f32  c_coeff      = vec3LenSquare(base_to_vert) - 1.0F;

            f32 new_t = 0.0F;
            if (getLowestRoot(a_coeff, b_coeff, c_coeff, t, new_t)) {
                t         = new_t;
                found     = true;
                col_point = *verts[i];
            }
        }

        // -- Edge tests --
        // For each edge (v_a → v_b), we parameterize the closest point on
        // the infinite line as  v_a + f * edge, then solve for t where the
        // sphere (radius 1) touches that line.  Finally check f ∈ [0, 1].
        //
        // The quadratic coefficients are derived from expanding:
        //   |base + t*vel - (v_a + f*edge)|^2 = 1
        // where f = (edgeDotBaseToVert + t * edgeDotVel) / edgeSqLen
        // and substituting back.  The result is:
        //
        //   a = edgeSqLen * velSq        - edgeDotVel^2
        //   b = edgeSqLen * 2*dot(vel,btv) - 2 * edgeDotVel * edgeDotBtv
        //   c = edgeSqLen * (|btv|^2 - 1) - edgeDotBtv^2
        //
        // with btv = basePoint - v_a.

        struct Edge
        {
            const Vec3 *va;
            const Vec3 *vb;
        };
        Edge edges[3] = {{&p1, &p2}, {&p2, &p3}, {&p3, &p1}};

        for (int i = 0; i < 3; ++i) {
            Vec3 edge          = *edges[i].vb - *edges[i].va;
            Vec3 base_to_vert  = packet.base_point - *edges[i].va;
            f32  edge_sq_len   = vec3LenSquare(edge);
            f32  edge_dot_vel  = vec3Dot(edge, packet.velocity);
            f32  edge_dot_btv  = vec3Dot(edge, base_to_vert);

            f32 a_coeff = edge_sq_len * vel_sq - edge_dot_vel * edge_dot_vel;
            f32 b_coeff = edge_sq_len * (2.0F * vec3Dot(packet.velocity, base_to_vert)) -
                          2.0F * edge_dot_vel * edge_dot_btv;
            f32 c_coeff = edge_sq_len * (vec3LenSquare(base_to_vert) - 1.0F) -
                          edge_dot_btv * edge_dot_btv;

            f32 new_t = 0.0F;
            if (getLowestRoot(a_coeff, b_coeff, c_coeff, t, new_t)) {
                // Check that the hit point is actually on the edge segment [0,1].
                f32 f_param = (edge_dot_btv + new_t * edge_dot_vel) / edge_sq_len;
                if (f_param >= 0.0F && f_param <= 1.0F) {
                    t         = new_t;
                    found     = true;
                    col_point = *edges[i].va + edge * f_param;
                }
            }
        }
    }

    // ---- Record closest collision ----
    if (found) {
        f32 dist_to_col = t * vec3Length(packet.velocity);
        if (!packet.found_collision || dist_to_col < packet.nearest_distance) {
            packet.found_collision    = true;
            packet.nearest_distance   = dist_to_col;
            packet.intersection_point = col_point;
        }
    }
}

// ============================================================================
// Fauerby core — collideWithWorld()
//
// Recursive function that slides the unit sphere through the triangle soup.
// Each recursion handles one collision + slide response, up to
// MAX_RECURSION_DEPTH iterations.
//
// Returns the final eSpace position after all slides.
// ============================================================================

Vec3 CollisionWorld::collideWithWorld(CollisionPacket &packet,
                                     const Vec3 &pos,
                                     const Vec3 &vel,
                                     i32 depth) const
{
    if (depth > MAX_RECURSION_DEPTH) {
        return pos;
    }

    // Fresh sweep for this recursion level.
    packet.velocity            = vel;
    packet.normalized_velocity = vec3SafeNormalize(vel);
    packet.base_point          = pos;
    packet.found_collision     = false;
    packet.nearest_distance    = 0.0F;

    // ---- Broadphase note ----
    // For large worlds, first cull triangles by bounding sphere or
    // grid cell lookup here.  For PS1-era counts, brute force is fine.

    // Test every triangle in eSpace.
    for (const Triangle &tri : m_triangles) {
        Vec3 ep1 = vec3CompDiv(tri.a, packet.e_radius);
        Vec3 ep2 = vec3CompDiv(tri.b, packet.e_radius);
        Vec3 ep3 = vec3CompDiv(tri.c, packet.e_radius);
        checkTriangle(packet, ep1, ep2, ep3);
    }

    // ---- No collision: move freely ----
    if (!packet.found_collision) {
        return pos + vel;
    }

    // ---- Collision response (Fauerby Section 3.5) ----
    //
    // 1. Move to just before the collision point, keeping a tiny gap
    //    (VERY_CLOSE_DIST) so we don't start the next sweep embedded.
    // 2. Build a "slide plane" from the collision.
    // 3. Project the remaining velocity onto the slide plane.
    // 4. Recurse with the projected velocity.

    Vec3 destination = pos + vel;

    Vec3 new_base = pos;
    if (packet.nearest_distance >= VERY_CLOSE_DIST) {
        // Move as far as we can, minus the safety margin.
        f32  move_dist = packet.nearest_distance - VERY_CLOSE_DIST;
        Vec3 move_vec  = vec3SafeNormalize(vel) * move_dist;
        new_base = pos + move_vec;

        // Adjust the intersection point to account for the safety margin.
        // The slide plane origin must sit on the surface we actually reached.
        packet.intersection_point =
            packet.intersection_point - vec3SafeNormalize(vel) * VERY_CLOSE_DIST;
    }

    // Slide plane: the normal points from the collision surface toward the
    // sphere center.  This is the direction the sphere was "pushed" in.
    Vec3 slide_origin = packet.intersection_point;
    Vec3 slide_normal = vec3SafeNormalize(new_base - packet.intersection_point);

    // Store world-space collision normal for ground detection.
    packet.last_collision_normal = vec3SafeNormalize(
        vec3CompMul(slide_normal, packet.e_radius));

    // Project the original destination onto the slide plane.
    // This gives us the remaining velocity that runs parallel to the surface.
    f32  signed_d = vec3Dot(destination - slide_origin, slide_normal);
    Vec3 new_dest = destination - slide_normal * signed_d;
    Vec3 new_vel  = new_dest - new_base;

    // If remaining velocity is negligible, stop.
    if (vec3LenSquare(new_vel) < COLLISION_EPSILON * COLLISION_EPSILON) {
        return new_base;
    }

    return collideWithWorld(packet, new_base, new_vel, depth + 1);
}

// ============================================================================
// Fauerby core — collideAndSlide() (public entry point)
//
// Transforms from R3 (world) into eSpace, runs the recursive solver,
// then transforms back.
// ============================================================================

Vec3 CollisionWorld::collideAndSlide(CollisionPacket &packet,
                                     const Vec3 &position,
                                     const Vec3 &velocity)
{
    // Store R3 values for reference.
    packet.r3_position = position;
    packet.r3_velocity = velocity;

    // Transform to eSpace: divide all positions/velocities by the
    // ellipsoid radius on each axis.  In this space the ellipsoid
    // becomes a unit sphere.
    Vec3 e_pos = vec3CompDiv(position, packet.e_radius);
    Vec3 e_vel = vec3CompDiv(velocity, packet.e_radius);

    // Run the recursive collide-and-slide in eSpace.
    Vec3 final_e_pos = collideWithWorld(packet, e_pos, e_vel, 0);

    // Transform back to R3.
    return vec3CompMul(final_e_pos, packet.e_radius);
}

// ============================================================================
// Query — Raycast (Moller-Trumbore)
// ============================================================================

bool CollisionWorld::rayTriangle(Vec3 origin, Vec3 dir, const Triangle &tri,
                                 f32 max_dist, f32 &out_t, Vec3 &out_point)
{
    Vec3 e1  = tri.b - tri.a;
    Vec3 e2  = tri.c - tri.a;
    Vec3 pvec = vec3Cross(dir, e2);
    f32  det  = vec3Dot(e1, pvec);

    if (fabsf(det) < COLLISION_EPSILON) {
        return false;
    }

    f32  inv_det = 1.0F / det;
    Vec3 tvec    = origin - tri.a;
    f32  u       = vec3Dot(tvec, pvec) * inv_det;
    if (u < 0.0F || u > 1.0F) {
        return false;
    }

    Vec3 qvec = vec3Cross(tvec, e1);
    f32  v    = vec3Dot(dir, qvec) * inv_det;
    if (v < 0.0F || u + v > 1.0F) {
        return false;
    }

    f32 t = vec3Dot(e2, qvec) * inv_det;
    if (t < 0.0F || t > max_dist) {
        return false;
    }

    out_t     = t;
    out_point = origin + dir * t;
    return true;
}

bool CollisionWorld::raycast(Vec3 origin, Vec3 dir, f32 max_dist, RaycastHit &out_hit) const
{
    dir = vec3SafeNormalize(dir);

    bool hit           = false;
    f32  closest_t     = max_dist;

    for (const Triangle &tri : m_triangles) {
        f32  t     = 0.0F;
        Vec3 point = {0, 0, 0};
        if (rayTriangle(origin, dir, tri, closest_t, t, point)) {
            closest_t        = t;
            out_hit.point    = point;
            out_hit.normal   = tri.normal;
            out_hit.distance = t;
            hit              = true;
        }
    }

    return hit;
}

// ============================================================================
// Query — Sphere sweep
//
// Shrinks the sphere radius to zero and inflates the triangles instead
// (Minkowski sum approach), then does a raycast.  For PS1-poly counts
// this is fast enough.  For a more precise sweep, use collideAndSlide
// with a uniform eRadius.
// ============================================================================

bool CollisionWorld::sphereSweep(Vec3 start, Vec3 end, f32 radius, SweepHit &out_hit) const
{
    // Use the Fauerby solver with a uniform sphere as the ellipsoid.
    CollisionPacket packet;
    packet.e_radius = {radius, radius, radius};

    // We need a mutable copy of the world for collideAndSlide.
    // Since sphereSweep is const, we use a simpler approach: for each tri,
    // do a swept-sphere vs triangle test.  We approximate by checking if
    // a ray from start→end hits the Minkowski-expanded triangle.

    Vec3 dir    = end - start;
    f32  dist   = vec3Length(dir);
    if (dist < COLLISION_EPSILON) {
        return false;
    }
    Vec3 dir_n  = dir / dist;

    bool hit       = false;
    f32  closest_t = dist;

    for (const Triangle &tri : m_triangles) {
        // Expand triangle by sphere radius along its normal.
        Vec3 offset = tri.normal * radius;
        Vec3 a_exp  = tri.a + offset;
        Vec3 b_exp  = tri.b + offset;
        Vec3 c_exp  = tri.c + offset;

        Triangle exp_tri;
        exp_tri.a      = a_exp;
        exp_tri.b      = b_exp;
        exp_tri.c      = c_exp;
        exp_tri.normal = tri.normal;

        f32  t     = 0.0F;
        Vec3 point = {0, 0, 0};
        if (rayTriangle(start, dir_n, exp_tri, closest_t, t, point)) {
            closest_t       = t;
            out_hit.point   = start + dir_n * t;
            out_hit.normal  = tri.normal;
            out_hit.t       = t / dist;
            hit             = true;
        }
    }

    return hit;
}

// ============================================================================
// Query — Capsule sweep (uses collideAndSlide with matching ellipsoid)
// ============================================================================

bool CollisionWorld::capsuleSweep(Vec3 start, Vec3 end, Vec3 e_radius, SweepHit &out_hit) const
{
    CollisionPacket packet;
    packet.e_radius = e_radius;

    // We can't call collideAndSlide (non-const) from a const method, and
    // we don't want sliding — we want the first hit.  Use a simplified
    // single-pass check.

    Vec3 vel = end - start;
    f32  vel_len = vec3Length(vel);
    if (vel_len < COLLISION_EPSILON) {
        return false;
    }

    // Transform to eSpace.
    Vec3 e_pos = vec3CompDiv(start, e_radius);
    Vec3 e_vel = vec3CompDiv(vel, e_radius);

    packet.base_point          = e_pos;
    packet.velocity            = e_vel;
    packet.normalized_velocity = vec3SafeNormalize(e_vel);
    packet.found_collision     = false;
    packet.nearest_distance    = 0.0F;

    for (const Triangle &tri : m_triangles) {
        Vec3 ep1 = vec3CompDiv(tri.a, e_radius);
        Vec3 ep2 = vec3CompDiv(tri.b, e_radius);
        Vec3 ep3 = vec3CompDiv(tri.c, e_radius);
        checkTriangle(packet, ep1, ep2, ep3);
    }

    if (!packet.found_collision) {
        return false;
    }

    Vec3 e_hit_point = packet.intersection_point;
    out_hit.point  = vec3CompMul(e_hit_point, e_radius);
    out_hit.normal = vec3SafeNormalize(
        vec3CompMul(vec3SafeNormalize(e_pos - e_hit_point), e_radius));
    out_hit.t = packet.nearest_distance / vec3Length(e_vel);

    return true;
}

// ============================================================================
// Query — AABB overlap (triangle-AABB intersection)
// ============================================================================

bool CollisionWorld::overlapAABB(const Vec3 &box_min, const Vec3 &box_max) const
{
    // Conservative test: check if any triangle vertex is inside the AABB,
    // or if any triangle edge intersects an AABB face.
    // For simplicity, use the separating-axis theorem (SAT) lite:
    // check triangle AABB vs query AABB first, then vertex containment.

    for (const Triangle &tri : m_triangles) {
        Vec3 tri_min = vec3Min(tri.a, vec3Min(tri.b, tri.c));
        Vec3 tri_max = vec3Max(tri.a, vec3Max(tri.b, tri.c));

        // AABB vs AABB overlap test.
        if (tri_max.x < box_min.x || tri_min.x > box_max.x) continue;
        if (tri_max.y < box_min.y || tri_min.y > box_max.y) continue;
        if (tri_max.z < box_min.z || tri_min.z > box_max.z) continue;

        return true;  // conservative: AABBs overlap → assume collision
    }

    return false;
}

// ============================================================================
// Query — Sphere overlap
// ============================================================================

bool CollisionWorld::overlapSphere(Vec3 center, f32 radius) const
{
    f32 r_sq = radius * radius;

    for (const Triangle &tri : m_triangles) {
        // Closest point on triangle to sphere center.
        // Project center onto triangle plane, then clamp to triangle.
        // Simplified: check distance to each vertex and edge.

        // Quick check: distance from center to triangle plane.
        f32 plane_dist = fabsf(vec3Dot(center - tri.a, tri.normal));
        if (plane_dist > radius) continue;

        // Check if projection is inside triangle.
        Vec3 proj = center - tri.normal * vec3Dot(center - tri.a, tri.normal);
        if (pointInTriangle(proj, tri.a, tri.b, tri.c)) {
            return true;
        }

        // Check distance to vertices.
        if (vec3LenSquare(center - tri.a) <= r_sq) return true;
        if (vec3LenSquare(center - tri.b) <= r_sq) return true;
        if (vec3LenSquare(center - tri.c) <= r_sq) return true;

        // Check distance to edges (closest point on segment).
        auto closestOnSegment = [](Vec3 pt, Vec3 seg_a, Vec3 seg_b) -> Vec3 {
            Vec3 ab = seg_b - seg_a;
            f32  t  = vec3Dot(pt - seg_a, ab) / vec3LenSquare(ab);
            t       = clampf(t, 0.0F, 1.0F);
            return seg_a + ab * t;
        };

        Vec3 cp1 = closestOnSegment(center, tri.a, tri.b);
        if (vec3LenSquare(center - cp1) <= r_sq) return true;
        Vec3 cp2 = closestOnSegment(center, tri.b, tri.c);
        if (vec3LenSquare(center - cp2) <= r_sq) return true;
        Vec3 cp3 = closestOnSegment(center, tri.c, tri.a);
        if (vec3LenSquare(center - cp3) <= r_sq) return true;
    }

    return false;
}

// ============================================================================
// Query — Point in liquid
// ============================================================================

bool CollisionWorld::pointInLiquid(Vec3 pos, f32 &out_water_height) const
{
    for (const LiquidVolume &liq : m_liquids) {
        if (pos.x >= liq.min.x && pos.x <= liq.max.x &&
            pos.z >= liq.min.z && pos.z <= liq.max.z &&
            pos.y <= liq.surface_height && pos.y >= liq.min.y)
        {
            out_water_height = liq.surface_height;
            return true;
        }
    }
    return false;
}

// ============================================================================
// Trigger testing
// ============================================================================

void CollisionWorld::checkTriggers(Vec3 position, f32 radius)
{
    for (TriggerVolume &trigger : m_triggers) {
        if (!trigger.active || !trigger.on_enter) continue;

        bool overlaps = false;

        if (trigger.shape == TriggerShape::AABB) {
            // Sphere-AABB overlap.
            Vec3 closest = vec3Max(trigger.min, vec3Min(position, trigger.max));
            overlaps = vec3LenSquare(position - closest) <= radius * radius;
        } else {
            // Sphere-sphere overlap.
            f32 dist_sq  = vec3LenSquare(position - trigger.center);
            f32 combined = radius + trigger.radius;
            overlaps     = dist_sq <= combined * combined;
        }

        if (overlaps) {
            trigger.on_enter(trigger.id);
        }
    }
}

// ============================================================================
// Kinematic body update (simplified slide for enemies / props)
// ============================================================================

void CollisionWorld::updateKinematics(f32 dt)
{
    for (KinematicBody &body : m_kinematics) {
        if (!body.active) continue;

        Vec3 vel = body.velocity * dt;
        if (vec3LenSquare(vel) < COLLISION_EPSILON * COLLISION_EPSILON) continue;

        CollisionPacket packet;
        packet.e_radius = body.e_radius;
        body.position = collideAndSlide(packet, body.position, vel);
    }
}

// ============================================================================
// PlayerController
// ============================================================================

Vec3 PlayerController::currentERadius() const
{
    return {radius, crouching ? crouch_height : stand_height, radius};
}

Vec3 PlayerController::getPosition() const
{
    return position;
}

Vec3 PlayerController::getEyePosition() const
{
    f32 eye_y = crouching ? eye_offset_crouch : eye_offset_stand;
    return {position.x, position.y + eye_y, position.z};
}

// ---------------------------------------------------------------------------
// PlayerController::update()
//
// Two-pass collide-and-slide:
//   Pass 1 — horizontal movement (input + strafe).  Slides along walls.
//   Pass 2 — vertical movement (gravity + jump).   Slides along slopes.
//
// Separating passes prevents gravity from "pulling" the player along walls
// and keeps wall-sliding crisp.
//
// After both passes: ground check, step-up, water detection.
// ---------------------------------------------------------------------------

void PlayerController::update(CollisionWorld &world, f32 dt, Vec3 input_move, bool jump, bool crouch)
{
    // ---- Crouch ----
    crouching = crouch && grounded;

    Vec3 e_rad = currentERadius();

    // ---- Horizontal velocity from input ----
    Vec3 move_vel = input_move * move_speed;

    // ---- Jump ----
    if (jump && grounded) {
        velocity.y = jump_force;
        grounded   = false;
    }

    // ---- Gravity ----
    if (!grounded) {
        velocity.y += gravity * dt;
    }

    // ---- Water buoyancy ----
    f32 water_h = 0.0F;
    in_water = world.pointInLiquid(position, water_h);
    if (in_water) {
        water_height = water_h;
        f32 depth    = water_h - position.y;
        if (depth > 0.0F) {
            // Simple buoyancy: reduce gravity effect + dampen velocity.
            velocity.y += 8.0F * dt;
            velocity.y *= 0.95F;
            move_vel   = move_vel * 0.6F;  // slower in water
        }
    }

    // ---- Pass 1: horizontal movement ----
    CollisionPacket packet;
    packet.e_radius = e_rad;

    Vec3 h_vel = {move_vel.x * dt, 0.0F, move_vel.z * dt};
    if (vec3LenSquare(h_vel) > COLLISION_EPSILON * COLLISION_EPSILON) {
        position = world.collideAndSlide(packet, position, h_vel);
    }

    // ---- Step-up logic ----
    // If horizontal movement was blocked and we're on the ground, try
    // stepping up (common for stairs / small ledges).
    if (grounded && packet.found_collision && vec3LenSquare(h_vel) > COLLISION_EPSILON) {
        tryStepUp(world, h_vel, dt);
    }

    // ---- Pass 2: vertical movement (gravity) ----
    CollisionPacket grav_packet;
    grav_packet.e_radius = e_rad;

    Vec3 g_vel = {0.0F, velocity.y * dt, 0.0F};
    position = world.collideAndSlide(grav_packet, position, g_vel);

    // ---- Ground detection ----
    // After the gravity pass, if we collided and the surface normal points
    // mostly upward, we're standing on ground.
    if (grav_packet.found_collision) {
        Vec3 norm = grav_packet.last_collision_normal;
        if (norm.y > GROUND_SLOPE_LIMIT) {
            grounded      = true;
            ground_normal = norm;
            velocity.y    = 0.0F;  // stop falling
        } else {
            grounded = false;
        }
    } else {
        grounded = false;
    }

    // ---- Trigger check ----
    world.checkTriggers(position, radius);
}

// ---------------------------------------------------------------------------
// Step-up: try moving up → forward → down.
// If the final position is further forward than the blocked position, use it.
// This makes stairs and small ledges feel smooth.
// ---------------------------------------------------------------------------

void PlayerController::tryStepUp(CollisionWorld &world, const Vec3 &move_vel, f32 /*dt*/)
{
    Vec3 e_rad    = currentERadius();
    Vec3 saved    = position;

    // 1. Move up by step_height.
    CollisionPacket up_packet;
    up_packet.e_radius = e_rad;
    Vec3 up_pos = world.collideAndSlide(up_packet, position, {0.0F, step_height, 0.0F});

    // 2. Move forward (same horizontal velocity).
    CollisionPacket fwd_packet;
    fwd_packet.e_radius = e_rad;
    Vec3 fwd_pos = world.collideAndSlide(fwd_packet, up_pos, move_vel);

    // 3. Move back down.
    CollisionPacket down_packet;
    down_packet.e_radius = e_rad;
    Vec3 down_pos = world.collideAndSlide(down_packet, fwd_pos, {0.0F, -step_height * 2.0F, 0.0F});

    // Accept if we moved further horizontally than the original blocked position.
    Vec3 horiz_old  = {saved.x, 0.0F, saved.z};
    Vec3 horiz_new  = {down_pos.x, 0.0F, down_pos.z};
    Vec3 horiz_orig = {position.x, 0.0F, position.z};

    if (vec3LenSquare(horiz_new - horiz_old) > vec3LenSquare(horiz_orig - horiz_old) + COLLISION_EPSILON) {
        position = down_pos;

        // Check if we landed on ground.
        if (down_packet.found_collision && down_packet.last_collision_normal.y > GROUND_SLOPE_LIMIT) {
            grounded      = true;
            ground_normal = down_packet.last_collision_normal;
            velocity.y    = 0.0F;
        }
    }
}

// ============================================================================
// Debug draw — wireframe collision triangles
// ============================================================================

void CollisionWorld::debugBuild()
{
    if (m_triangles.empty()) return;

    // 3 edges per triangle, 2 vertices per edge, 3 floats per vertex.
    m_debug_line_count = static_cast<u32>(m_triangles.size()) * 6;
    std::vector<f32> lines;
    lines.reserve(static_cast<usize>(m_debug_line_count) * 3);

    for (const Triangle &tri : m_triangles) {
        // Edge A→B
        lines.push_back(tri.a.x); lines.push_back(tri.a.y); lines.push_back(tri.a.z);
        lines.push_back(tri.b.x); lines.push_back(tri.b.y); lines.push_back(tri.b.z);
        // Edge B→C
        lines.push_back(tri.b.x); lines.push_back(tri.b.y); lines.push_back(tri.b.z);
        lines.push_back(tri.c.x); lines.push_back(tri.c.y); lines.push_back(tri.c.z);
        // Edge C→A
        lines.push_back(tri.c.x); lines.push_back(tri.c.y); lines.push_back(tri.c.z);
        lines.push_back(tri.a.x); lines.push_back(tri.a.y); lines.push_back(tri.a.z);
    }

    if (m_debug_vao == 0) {
        glGenVertexArrays(1, &m_debug_vao);
        glGenBuffers(1, &m_debug_vbo);
    }

    glBindVertexArray(m_debug_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_debug_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(lines.size() * sizeof(f32)),
                 lines.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void CollisionWorld::debugDraw(u32 debug_shader, const Matrix4 &view_proj) const
{
    if (m_debug_vao == 0 || m_debug_line_count == 0) return;

    glUseProgram(debug_shader);

    // Assume the shader has a uniform "u_view_proj" at location 0.
    GLint loc = glGetUniformLocation(debug_shader, "u_view_proj");
    if (loc >= 0) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, view_proj.data.data());
    }

    glBindVertexArray(m_debug_vao);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_debug_line_count));
    glBindVertexArray(0);
}

void CollisionWorld::debugDestroy()
{
    if (m_debug_vao != 0) {
        glDeleteVertexArrays(1, &m_debug_vao);
        glDeleteBuffers(1, &m_debug_vbo);
        m_debug_vao        = 0;
        m_debug_vbo        = 0;
        m_debug_line_count = 0;
    }
}

}  // namespace chad

// ============================================================================
// Example usage (pseudocode — paste into your game loop)
// ============================================================================
//
//   #include <engine/physics/collision.h>
//
//   chad::CollisionWorld world;
//   chad::PlayerController player;
//
//   // --- Setup (after generating dungeon mesh) ---
//   // Convert your dungeon vertices + indices into Triangles:
//   std::vector<chad::Triangle> tris;
//   for (u32 i = 0; i < index_count; i += 3) {
//       tris.push_back(chad::makeTriangle(
//           verts[indices[i+0]], verts[indices[i+1]], verts[indices[i+2]]));
//   }
//   world.addTriangles(tris);
//   world.debugBuild();  // optional: for wireframe visualization
//
//   // Add a water pool:
//   chad::LiquidVolume pool;
//   pool.min = {10, 0, 10};
//   pool.max = {20, 3, 20};
//   pool.surface_height = 2.5f;
//   world.addLiquid(pool);
//
//   // Add a trap trigger:
//   chad::TriggerVolume trap;
//   trap.shape = chad::TriggerShape::AABB;
//   trap.min   = {5, 0, 5};
//   trap.max   = {6, 2, 6};
//   trap.id    = 42;
//   trap.on_enter = [](u32 id) { LOG_INFO("Trap %u triggered!", id); };
//   world.addTrigger(trap);
//
//   // --- Per frame ---
//   float dt = timerDelta(timer);
//   Vec3 input = ...; // from WASD relative to camera forward
//   bool jump  = inputKeyPressed(KEY_SPACE);
//   bool crouch = inputKeyDown(KEY_LCTRL);
//
//   player.update(world, dt, input, jump, crouch);
//
//   // Use player.getEyePosition() for camera.
//   cam.position = player.getEyePosition();
//
//   // Melee attack:
//   chad::SweepHit melee_hit;
//   Vec3 swing_start = player.getEyePosition();
//   Vec3 swing_end   = swing_start + cameraForward(cam) * 2.0f;
//   if (world.sphereSweep(swing_start, swing_end, 0.3f, melee_hit)) {
//       // Deal damage at melee_hit.point
//   }
//
//   // Hitscan spell:
//   chad::RaycastHit ray_hit;
//   if (world.raycast(cam.position, cameraForward(cam), 100.0f, ray_hit)) {
//       // Spawn impact VFX at ray_hit.point
//   }
//
//   // Debug wireframe:
//   world.debugDraw(debugShaderProgram, viewProj);
//
