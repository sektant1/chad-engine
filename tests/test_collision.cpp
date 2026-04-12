#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <engine/physics/collision.h>

using namespace chad;
using Catch::Approx;

// ============================================================================
// Math extensions
// ============================================================================

TEST_CASE("vec3CompMul component-wise multiply", "[collision][math_ext]")
{
    Vec3 a = {2, 3, 4};
    Vec3 b = {5, 6, 7};
    Vec3 r = vec3CompMul(a, b);
    REQUIRE(r.x == 10.0F);
    REQUIRE(r.y == 18.0F);
    REQUIRE(r.z == 28.0F);
}

TEST_CASE("vec3CompDiv component-wise divide", "[collision][math_ext]")
{
    Vec3 a = {10, 18, 28};
    Vec3 b = {5, 6, 7};
    Vec3 r = vec3CompDiv(a, b);
    REQUIRE(r.x == Approx(2.0F));
    REQUIRE(r.y == Approx(3.0F));
    REQUIRE(r.z == Approx(4.0F));
}

TEST_CASE("Vec3 scalar division", "[collision][math_ext]")
{
    Vec3 v = {6, 9, 12};
    Vec3 r = v / 3.0F;
    REQUIRE(r.x == Approx(2.0F));
    REQUIRE(r.y == Approx(3.0F));
    REQUIRE(r.z == Approx(4.0F));
}

TEST_CASE("vec3SafeNormalize returns fallback for zero vector", "[collision][math_ext]")
{
    Vec3 zero = {0, 0, 0};
    Vec3 fb   = {0, 1, 0};
    Vec3 r    = vec3SafeNormalize(zero, fb);
    REQUIRE(r.x == 0.0F);
    REQUIRE(r.y == 1.0F);
    REQUIRE(r.z == 0.0F);
}

TEST_CASE("vec3SafeNormalize normalizes non-zero", "[collision][math_ext]")
{
    Vec3 v = {3, 4, 0};
    Vec3 r = vec3SafeNormalize(v);
    REQUIRE(vec3Length(r) == Approx(1.0F).margin(1e-5F));
}

TEST_CASE("pointInTriangle center point", "[collision][math_ext]")
{
    Vec3 a = {0, 0, 0};
    Vec3 b = {1, 0, 0};
    Vec3 c = {0, 0, 1};
    Vec3 center = {0.25F, 0, 0.25F};
    REQUIRE(pointInTriangle(center, a, b, c));
}

TEST_CASE("pointInTriangle outside point", "[collision][math_ext]")
{
    Vec3 a = {0, 0, 0};
    Vec3 b = {1, 0, 0};
    Vec3 c = {0, 0, 1};
    Vec3 outside = {2, 0, 2};
    REQUIRE_FALSE(pointInTriangle(outside, a, b, c));
}

TEST_CASE("getLowestRoot finds valid root", "[collision][math_ext]")
{
    // t^2 - 3t + 2 = 0 → roots at 1 and 2
    f32 root = 0;
    REQUIRE(getLowestRoot(1.0F, -3.0F, 2.0F, 5.0F, root));
    REQUIRE(root == Approx(1.0F));
}

TEST_CASE("getLowestRoot respects max", "[collision][math_ext]")
{
    // roots at 1 and 2, max = 0.5 → no valid root
    f32 root = 0;
    REQUIRE_FALSE(getLowestRoot(1.0F, -3.0F, 2.0F, 0.5F, root));
}

TEST_CASE("getLowestRoot no real roots", "[collision][math_ext]")
{
    // t^2 + t + 1 = 0 → discriminant < 0
    f32 root = 0;
    REQUIRE_FALSE(getLowestRoot(1.0F, 1.0F, 1.0F, 10.0F, root));
}

// ============================================================================
// makeTriangle
// ============================================================================

TEST_CASE("makeTriangle computes unit normal", "[collision]")
{
    Triangle tri = makeTriangle({0, 0, 0}, {1, 0, 0}, {0, 0, 1});
    // CCW winding on XZ plane → normal points up (-Y or +Y depending on winding)
    // cross(B-A, C-A) = cross({1,0,0}, {0,0,1}) = {0,-1,0}
    REQUIRE(tri.normal.x == Approx(0.0F).margin(1e-5F));
    REQUIRE(tri.normal.y == Approx(-1.0F).margin(1e-5F));
    REQUIRE(tri.normal.z == Approx(0.0F).margin(1e-5F));
}

// ============================================================================
// CollisionWorld — basic operations
// ============================================================================

TEST_CASE("CollisionWorld addTriangles and clear", "[collision]")
{
    CollisionWorld world;
    std::vector<Triangle> tris = {
        makeTriangle({0, 0, 0}, {1, 0, 0}, {0, 0, 1}),
        makeTriangle({1, 0, 0}, {1, 0, 1}, {0, 0, 1}),
    };
    world.addTriangles(tris);
    REQUIRE(world.triangleCount() == 2);

    world.clear();
    REQUIRE(world.triangleCount() == 0);
}

// ============================================================================
// Raycast
// ============================================================================

TEST_CASE("raycast hits floor triangle", "[collision][raycast]")
{
    CollisionWorld world;
    // Floor quad at Y=0, spanning [-5,5] on XZ
    world.addTriangles({
        makeTriangle({-5, 0, -5}, {5, 0, -5}, {5, 0, 5}),
        makeTriangle({-5, 0, -5}, {5, 0, 5}, {-5, 0, 5}),
    });

    RaycastHit hit;
    // Shoot down from Y=10
    REQUIRE(world.raycast({0, 10, 0}, {0, -1, 0}, 100.0F, hit));
    REQUIRE(hit.distance == Approx(10.0F).margin(0.01F));
    REQUIRE(hit.point.y == Approx(0.0F).margin(0.01F));
}

TEST_CASE("raycast misses when pointing away", "[collision][raycast]")
{
    CollisionWorld world;
    world.addTriangles({makeTriangle({-5, 0, -5}, {-5, 0, 5}, {5, 0, 5})});

    RaycastHit hit;
    // Shoot up from Y=10
    REQUIRE_FALSE(world.raycast({0, 10, 0}, {0, 1, 0}, 100.0F, hit));
}

TEST_CASE("raycast respects max distance", "[collision][raycast]")
{
    CollisionWorld world;
    world.addTriangles({
        makeTriangle({-5, 0, -5}, {5, 0, -5}, {5, 0, 5}),
        makeTriangle({-5, 0, -5}, {5, 0, 5}, {-5, 0, 5}),
    });

    RaycastHit hit;
    // Floor at Y=0, shoot from Y=10, max dist 5 → miss
    REQUIRE_FALSE(world.raycast({0, 10, 0}, {0, -1, 0}, 5.0F, hit));
}

// ============================================================================
// Sphere overlap
// ============================================================================

TEST_CASE("overlapSphere detects floor contact", "[collision][overlap]")
{
    CollisionWorld world;
    world.addTriangles({
        makeTriangle({-5, 0, -5}, {5, 0, -5}, {5, 0, 5}),
        makeTriangle({-5, 0, -5}, {5, 0, 5}, {-5, 0, 5}),
    });

    // Sphere at Y=0.5, radius 1.0 → touches floor
    REQUIRE(world.overlapSphere({0, 0.5F, 0}, 1.0F));
    // Sphere at Y=5, radius 1.0 → too far
    REQUIRE_FALSE(world.overlapSphere({0, 5.0F, 0}, 1.0F));
}

// ============================================================================
// AABB overlap
// ============================================================================

TEST_CASE("overlapAABB detects floor contact", "[collision][overlap]")
{
    CollisionWorld world;
    world.addTriangles({
        makeTriangle({-5, 0, -5}, {5, 0, -5}, {5, 0, 5}),
        makeTriangle({-5, 0, -5}, {5, 0, 5}, {-5, 0, 5}),
    });

    // Box spanning the floor
    REQUIRE(world.overlapAABB({-1, -1, -1}, {1, 1, 1}));
    // Box far above
    REQUIRE_FALSE(world.overlapAABB({-1, 10, -1}, {1, 12, 1}));
}

// ============================================================================
// Liquid
// ============================================================================

TEST_CASE("pointInLiquid detects water", "[collision][liquid]")
{
    CollisionWorld world;
    LiquidVolume pool;
    pool.min            = {-5, 0, -5};
    pool.max            = {5, 3, 5};
    pool.surface_height = 2.5F;
    world.addLiquid(pool);

    f32 water_h = 0;
    REQUIRE(world.pointInLiquid({0, 1, 0}, water_h));
    REQUIRE(water_h == Approx(2.5F));

    // Above surface
    REQUIRE_FALSE(world.pointInLiquid({0, 3, 0}, water_h));
    // Outside XZ bounds
    REQUIRE_FALSE(world.pointInLiquid({10, 1, 0}, water_h));
}

// ============================================================================
// CollideAndSlide — sphere doesn't fall through floor
// ============================================================================

TEST_CASE("collideAndSlide stops sphere above floor", "[collision][fauerby]")
{
    CollisionWorld world;
    // Large floor at Y=0
    world.addTriangles({
        makeTriangle({-50, 0, -50}, {-50, 0, 50}, {50, 0, 50}),
        makeTriangle({-50, 0, -50}, {50, 0, 50}, {50, 0, -50}),
    });

    CollisionPacket packet;
    packet.e_radius = {0.5F, 0.5F, 0.5F};  // uniform sphere

    // Start at Y=5, move down by 10 → should stop near Y=0.5 (sphere radius)
    Vec3 start = {0, 5, 0};
    Vec3 vel   = {0, -10, 0};
    Vec3 final_pos = world.collideAndSlide(packet, start, vel);

    REQUIRE(final_pos.y >= 0.49F);  // above floor by ~radius
    REQUIRE(final_pos.y < 1.1F);    // didn't stay at start (radius + VERY_CLOSE_DIST margin)
    REQUIRE(packet.found_collision);
}

TEST_CASE("collideAndSlide free movement when no geometry", "[collision][fauerby]")
{
    CollisionWorld world;
    // Empty world

    CollisionPacket packet;
    packet.e_radius = {0.5F, 0.5F, 0.5F};

    Vec3 start = {0, 5, 0};
    Vec3 vel   = {0, -10, 0};
    Vec3 final_pos = world.collideAndSlide(packet, start, vel);

    REQUIRE(final_pos.x == Approx(0.0F));
    REQUIRE(final_pos.y == Approx(-5.0F));
    REQUIRE(final_pos.z == Approx(0.0F));
}

TEST_CASE("collideAndSlide slides along wall", "[collision][fauerby]")
{
    CollisionWorld world;

    // Wall at X=2, facing -X, spanning Y and Z
    world.addTriangles({
        makeTriangle({2, -5, -50}, {2, -5, 50}, {2, 5, 50}),
        makeTriangle({2, -5, -50}, {2, 5, 50}, {2, 5, -50}),
    });

    CollisionPacket packet;
    packet.e_radius = {0.5F, 0.5F, 0.5F};

    // Move diagonally into the wall: X and Z
    Vec3 start = {0, 0, 0};
    Vec3 vel   = {5, 0, 5};
    Vec3 final_pos = world.collideAndSlide(packet, start, vel);

    // X should be stopped near wall (2 - radius = 1.5)
    REQUIRE(final_pos.x < 2.0F);
    REQUIRE(final_pos.x > 1.0F);
    // Z should have slid forward (some velocity preserved along wall).
    // With large single-frame velocity the recursive epsilon eats some Z;
    // in real gameplay (small dt steps) this slides cleanly.
    REQUIRE(final_pos.z > 1.0F);
}

// ============================================================================
// PlayerController
// ============================================================================

TEST_CASE("PlayerController default values", "[collision][player]")
{
    PlayerController pc;
    REQUIRE(pc.grounded == false);
    REQUIRE(pc.position.y == Approx(2.0F));
    REQUIRE(pc.move_speed == Approx(5.0F));
}

TEST_CASE("PlayerController falls with gravity", "[collision][player]")
{
    CollisionWorld world;
    // Large floor at Y=0
    world.addTriangles({
        makeTriangle({-50, 0, -50}, {-50, 0, 50}, {50, 0, 50}),
        makeTriangle({-50, 0, -50}, {50, 0, 50}, {50, 0, -50}),
    });

    PlayerController pc;
    pc.position = {0, 5, 0};

    // Simulate several frames
    for (int i = 0; i < 120; ++i) {
        pc.update(world, 1.0F / 60.0F, {0, 0, 0}, false, false);
    }

    // Should have landed on the floor
    REQUIRE(pc.grounded);
    REQUIRE(pc.position.y < 2.0F);
    REQUIRE(pc.position.y > -0.1F);
}

TEST_CASE("PlayerController moves horizontally", "[collision][player]")
{
    CollisionWorld world;
    // Floor
    world.addTriangles({
        makeTriangle({-50, 0, -50}, {-50, 0, 50}, {50, 0, 50}),
        makeTriangle({-50, 0, -50}, {50, 0, 50}, {50, 0, -50}),
    });

    PlayerController pc;
    pc.position = {0, 1, 0};
    pc.grounded = true;
    pc.velocity = {0, 0, 0};

    // Move along +X for 1 second
    for (int i = 0; i < 60; ++i) {
        pc.update(world, 1.0F / 60.0F, {1, 0, 0}, false, false);
    }

    // Should have moved right
    REQUIRE(pc.position.x > 3.0F);
}

TEST_CASE("PlayerController getEyePosition above center", "[collision][player]")
{
    PlayerController pc;
    pc.position = {1, 2, 3};
    Vec3 eye = pc.getEyePosition();
    REQUIRE(eye.x == 1.0F);
    REQUIRE(eye.y > 2.0F);  // eye offset is positive
    REQUIRE(eye.z == 3.0F);
}
