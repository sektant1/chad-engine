#pragma once

// ============================================================================
// Math extensions for collision detection
//
// Thin helpers on top of the engine's existing Vec3 / Matrix4 / f32 types.
// Every addition here exists because Fauerby's ellipsoid-collision algorithm
// needs it and the base math.h doesn't provide it.  Each function is
// annotated with *why* it matters for numerical stability.
// ============================================================================

#include <engine/core/math.h>

#include <cmath>
#include <algorithm>

namespace chad
{

// ---------------------------------------------------------------------------
// Epsilon constants (Fauerby Section 3.5 — "very close distance")
// ---------------------------------------------------------------------------

// Smallest meaningful distance for collision detection.
// Below this, floating-point noise dominates.
constexpr f32 COLLISION_EPSILON = 0.001F;

// Safety margin kept between the swept sphere and the surface after each
// collision response step.  Prevents the sphere from sinking into geometry
// on the next frame due to FP rounding.  0.005 is the value recommended
// in the original paper.
constexpr f32 VERY_CLOSE_DIST = 0.005F;

// Slope threshold for "ground" detection (dot with up vector).
// cos(46 deg) ≈ 0.7 — anything steeper is a wall, not a floor.
constexpr f32 GROUND_SLOPE_LIMIT = 0.7F;

// ---------------------------------------------------------------------------
// Component-wise Vec3 operations
// Needed to transform between R3 (real space) and eSpace (ellipsoid space).
// eSpace divides every coordinate by the ellipsoid radius on that axis.
// ---------------------------------------------------------------------------

inline Vec3 vec3CompMul(Vec3 a, Vec3 b)
{
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}

inline Vec3 vec3CompDiv(Vec3 a, Vec3 b)
{
    // No zero-check: caller guarantees eRadius > 0 on all axes.
    return {a.x / b.x, a.y / b.y, a.z / b.z};
}

// ---------------------------------------------------------------------------
// Scalar division for Vec3 (missing from base math.h)
// ---------------------------------------------------------------------------

inline Vec3 operator/(Vec3 v, f32 s)
{
    f32 inv = 1.0F / s;
    return {v.x * inv, v.y * inv, v.z * inv};
}

// ---------------------------------------------------------------------------
// Safe normalize — returns `fallback` instead of {0,0,0} for degenerate
// vectors.  Critical in the slide-plane calculation: if the sphere ends up
// exactly at the collision point, the slide normal would be zero, producing
// NaN velocities on the next recursion.
// ---------------------------------------------------------------------------

inline Vec3 vec3SafeNormalize(Vec3 v, Vec3 fallback = {0.0F, 1.0F, 0.0F})
{
    f32 len_sq = vec3LenSquare(v);
    if (len_sq < 1e-12F) {
        return fallback;
    }
    f32 inv = 1.0F / sqrtf(len_sq);
    return {v.x * inv, v.y * inv, v.z * inv};
}

// ---------------------------------------------------------------------------
// Per-component min / max (useful for AABB broadphase)
// ---------------------------------------------------------------------------

inline Vec3 vec3Min(Vec3 a, Vec3 b)
{
    return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
}

inline Vec3 vec3Max(Vec3 a, Vec3 b)
{
    return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)};
}

// ---------------------------------------------------------------------------
// Signed distance from a point to a plane (origin + normal form).
// Positive = same side as normal, negative = opposite side.
// ---------------------------------------------------------------------------

inline f32 signedDistToPlane(Vec3 point, Vec3 plane_origin, Vec3 plane_normal)
{
    return vec3Dot(point - plane_origin, plane_normal);
}

// ---------------------------------------------------------------------------
// Point-in-triangle test (barycentric coordinates).
// Uses a small epsilon tolerance to catch points sitting exactly on edges,
// which is common after the plane-intersection step in Fauerby's algorithm.
// ---------------------------------------------------------------------------

inline bool pointInTriangle(Vec3 p, Vec3 a, Vec3 b, Vec3 c)
{
    Vec3 v0 = c - a;
    Vec3 v1 = b - a;
    Vec3 v2 = p - a;

    f32 d00 = vec3Dot(v0, v0);
    f32 d01 = vec3Dot(v0, v1);
    f32 d02 = vec3Dot(v0, v2);
    f32 d11 = vec3Dot(v1, v1);
    f32 d12 = vec3Dot(v1, v2);

    f32 denom = d00 * d11 - d01 * d01;
    if (fabsf(denom) < 1e-10F) {
        return false;  // degenerate triangle
    }

    f32 inv = 1.0F / denom;
    f32 u   = (d11 * d02 - d01 * d12) * inv;
    f32 v   = (d00 * d12 - d01 * d02) * inv;

    // Small negative tolerance catches FP edge-cases.
    return (u >= -COLLISION_EPSILON) && (v >= -COLLISION_EPSILON) && (u + v <= 1.0F + COLLISION_EPSILON);
}

// ---------------------------------------------------------------------------
// Quadratic solver — Fauerby Section 3.2
//
// Solves  a*t^2 + b*t + c = 0  for the *lowest positive root < maxR*.
// Returns false when no valid root exists.
//
// Numerical notes:
//   - Discriminant must be >= 0 (no imaginary roots).
//   - Roots outside (0, maxR) are irrelevant (collision too far away).
//   - We test the smaller root first; if it's invalid, try the larger one.
//     This handles the swept sphere touching a vertex from behind.
// ---------------------------------------------------------------------------

inline bool getLowestRoot(f32 a, f32 b, f32 c, f32 max_r, f32 &root)
{
    f32 det = b * b - 4.0F * a * c;
    if (det < 0.0F) {
        return false;
    }

    f32 sqrt_det = sqrtf(det);
    f32 r1       = (-b - sqrt_det) / (2.0F * a);
    f32 r2       = (-b + sqrt_det) / (2.0F * a);

    // Ensure r1 <= r2
    if (r1 > r2) {
        f32 tmp = r1;
        r1      = r2;
        r2      = tmp;
    }

    if (r1 > 0.0F && r1 < max_r) {
        root = r1;
        return true;
    }
    if (r2 > 0.0F && r2 < max_r) {
        root = r2;
        return true;
    }

    return false;
}

}  // namespace chad
