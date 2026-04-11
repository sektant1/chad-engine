#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <engine/core/math.h>

#include <cmath>

using namespace chad;
using Catch::Approx;

// ---------------- scalar helpers ----------------

TEST_CASE("toRadians / toDegrees round-trip", "[math][scalar]")
{
    REQUIRE(toRadians(180.0F) == Approx(CHAD_PI));
    REQUIRE(toDegrees(CHAD_PI) == Approx(180.0F));
    REQUIRE(toDegrees(toRadians(90.0F)) == Approx(90.0F));
}

TEST_CASE("clampf bounds", "[math][scalar]")
{
    REQUIRE(clampf(5.0F, 0.0F, 10.0F) == 5.0F);
    REQUIRE(clampf(-1.0F, 0.0F, 10.0F) == 0.0F);
    REQUIRE(clampf(15.0F, 0.0F, 10.0F) == 10.0F);
    REQUIRE(clampf(0.0F, 0.0F, 10.0F) == 0.0F);
    REQUIRE(clampf(10.0F, 0.0F, 10.0F) == 10.0F);
}

// ---------------- Vec2 ----------------

TEST_CASE("Vec2 arithmetic", "[math][vec2]")
{
    Vec2 a {1.0F, 2.0F};
    Vec2 b {3.0F, 4.0F};

    Vec2 sum = a + b;
    REQUIRE(sum.x == 4.0F);
    REQUIRE(sum.y == 6.0F);

    Vec2 diff = b - a;
    REQUIRE(diff.x == 2.0F);
    REQUIRE(diff.y == 2.0F);

    Vec2 scaled = a * 2.5F;
    REQUIRE(scaled.x == Approx(2.5F));
    REQUIRE(scaled.y == Approx(5.0F));

    Vec2 rscaled = 2.0F * b;
    REQUIRE(rscaled.x == 6.0F);
    REQUIRE(rscaled.y == 8.0F);
}

// ---------------- Vec3 ----------------

TEST_CASE("Vec3 constructors", "[math][vec3]")
{
    Vec3 a = vec3(1.0F, 2.0F, 3.0F);
    REQUIRE(a.x == 1.0F);
    REQUIRE(a.y == 2.0F);
    REQUIRE(a.z == 3.0F);

    Vec3 s = vec3(5.0F);
    REQUIRE(s.x == 5.0F);
    REQUIRE(s.y == 5.0F);
    REQUIRE(s.z == 5.0F);
}

TEST_CASE("Vec3 basic arithmetic", "[math][vec3]")
{
    Vec3 a {1.0F, 2.0F, 3.0F};
    Vec3 b {4.0F, 5.0F, 6.0F};

    Vec3 sum = a + b;
    REQUIRE(sum.x == 5.0F);
    REQUIRE(sum.y == 7.0F);
    REQUIRE(sum.z == 9.0F);

    Vec3 diff = b - a;
    REQUIRE(diff.x == 3.0F);
    REQUIRE(diff.y == 3.0F);
    REQUIRE(diff.z == 3.0F);

    Vec3 neg = -a;
    REQUIRE(neg.x == -1.0F);
    REQUIRE(neg.y == -2.0F);
    REQUIRE(neg.z == -3.0F);

    Vec3 left  = a * 2.0F;
    Vec3 right = 2.0F * a;
    REQUIRE(left.x == right.x);
    REQUIRE(left.y == right.y);
    REQUIRE(left.z == right.z);
}

TEST_CASE("Vec3 compound assignment", "[math][vec3]")
{
    Vec3 a {1.0F, 2.0F, 3.0F};
    a += Vec3 {1.0F, 1.0F, 1.0F};
    REQUIRE(a.x == 2.0F);
    REQUIRE(a.y == 3.0F);
    REQUIRE(a.z == 4.0F);

    a -= Vec3 {1.0F, 1.0F, 1.0F};
    REQUIRE(a.x == 1.0F);
    REQUIRE(a.y == 2.0F);
    REQUIRE(a.z == 3.0F);

    a *= 3.0F;
    REQUIRE(a.x == 3.0F);
    REQUIRE(a.y == 6.0F);
    REQUIRE(a.z == 9.0F);
}

TEST_CASE("Vec3 dot product", "[math][vec3]")
{
    // orthogonal vectors → 0
    REQUIRE(vec3Dot({1, 0, 0}, {0, 1, 0}) == 0.0F);
    REQUIRE(vec3Dot({0, 1, 0}, {0, 0, 1}) == 0.0F);

    // parallel → product of lengths
    REQUIRE(vec3Dot({2, 0, 0}, {3, 0, 0}) == 6.0F);

    // general case
    REQUIRE(vec3Dot({1, 2, 3}, {4, 5, 6}) == Approx(32.0F));  // 4+10+18
}

TEST_CASE("Vec3 length / length squared", "[math][vec3]")
{
    Vec3 v {3.0F, 4.0F, 0.0F};
    REQUIRE(vec3LenSquare(v) == Approx(25.0F));
    REQUIRE(vec3Length(v) == Approx(5.0F));

    Vec3 unit {1, 0, 0};
    REQUIRE(vec3Length(unit) == Approx(1.0F));

    Vec3 zero {0, 0, 0};
    REQUIRE(vec3Length(zero) == Approx(0.0F));
}

TEST_CASE("Vec3 cross product", "[math][vec3]")
{
    Vec3 x {1, 0, 0};
    Vec3 y {0, 1, 0};
    Vec3 z = vec3Cross(x, y);
    REQUIRE(z.x == 0.0F);
    REQUIRE(z.y == 0.0F);
    REQUIRE(z.z == 1.0F);

    // anti-commutative
    Vec3 neg_z = vec3Cross(y, x);
    REQUIRE(neg_z.z == -1.0F);

    // cross with self → zero
    Vec3 zero = vec3Cross(x, x);
    REQUIRE(zero.x == 0.0F);
    REQUIRE(zero.y == 0.0F);
    REQUIRE(zero.z == 0.0F);
}

TEST_CASE("Vec3 normalize", "[math][vec3]")
{
    Vec3 v {3.0F, 4.0F, 0.0F};
    Vec3 n = vec3Normalize(v);
    REQUIRE(vec3Length(n) == Approx(1.0F));
    REQUIRE(n.x == Approx(0.6F));
    REQUIRE(n.y == Approx(0.8F));
    REQUIRE(n.z == Approx(0.0F));

    // zero vector → zero (guard against div-by-zero)
    Vec3 z = vec3Normalize({0, 0, 0});
    REQUIRE(z.x == 0.0F);
    REQUIRE(z.y == 0.0F);
    REQUIRE(z.z == 0.0F);
}

TEST_CASE("Vec3 lerp", "[math][vec3]")
{
    Vec3 a {0, 0, 0};
    Vec3 b {10, 20, 30};

    Vec3 at_zero = vec3Lerp(a, b, 0.0F);
    REQUIRE(at_zero.x == 0.0F);
    REQUIRE(at_zero.y == 0.0F);
    REQUIRE(at_zero.z == 0.0F);

    Vec3 at_one = vec3Lerp(a, b, 1.0F);
    REQUIRE(at_one.x == Approx(10.0F));
    REQUIRE(at_one.y == Approx(20.0F));
    REQUIRE(at_one.z == Approx(30.0F));

    Vec3 mid = vec3Lerp(a, b, 0.5F);
    REQUIRE(mid.x == Approx(5.0F));
    REQUIRE(mid.y == Approx(10.0F));
    REQUIRE(mid.z == Approx(15.0F));
}

// ---------------- Vec4 ----------------

TEST_CASE("Vec4 constructors", "[math][vec4]")
{
    Vec4 a = vec4(1.0F, 2.0F, 3.0F, 4.0F);
    REQUIRE(a.x == 1.0F);
    REQUIRE(a.w == 4.0F);

    Vec4 from3 = vec4(Vec3 {1, 2, 3}, 0.5F);
    REQUIRE(from3.x == 1.0F);
    REQUIRE(from3.y == 2.0F);
    REQUIRE(from3.z == 3.0F);
    REQUIRE(from3.w == 0.5F);
}

// ---------------- Matrix4 ----------------

TEST_CASE("matIdentity has diagonal 1s", "[math][mat4]")
{
    Matrix4 id = matIdentity();
    REQUIRE(id.data[0]  == 1.0F);
    REQUIRE(id.data[5]  == 1.0F);
    REQUIRE(id.data[10] == 1.0F);
    REQUIRE(id.data[15] == 1.0F);
    // off-diagonal
    REQUIRE(id.data[1] == 0.0F);
    REQUIRE(id.data[2] == 0.0F);
    REQUIRE(id.data[3] == 0.0F);
    REQUIRE(id.data[4] == 0.0F);
}

TEST_CASE("Matrix4 * identity == Matrix4", "[math][mat4]")
{
    Matrix4 t = matTranslate({1.0F, 2.0F, 3.0F});
    Matrix4 r = t * matIdentity();
    for (int i = 0; i < 16; ++i) {
        REQUIRE(r.data[i] == Approx(t.data[i]));
    }

    Matrix4 r2 = matIdentity() * t;
    for (int i = 0; i < 16; ++i) {
        REQUIRE(r2.data[i] == Approx(t.data[i]));
    }
}

TEST_CASE("matTranslate sets translation column", "[math][mat4]")
{
    Matrix4 t = matTranslate({5.0F, 6.0F, 7.0F});
    REQUIRE(t.data[12] == 5.0F);
    REQUIRE(t.data[13] == 6.0F);
    REQUIRE(t.data[14] == 7.0F);
    // upper 3x3 still identity
    REQUIRE(t.data[0] == 1.0F);
    REQUIRE(t.data[5] == 1.0F);
    REQUIRE(t.data[10] == 1.0F);
}

TEST_CASE("matScale diagonal", "[math][mat4]")
{
    Matrix4 s = matScale({2.0F, 3.0F, 4.0F});
    REQUIRE(s.data[0]  == 2.0F);
    REQUIRE(s.data[5]  == 3.0F);
    REQUIRE(s.data[10] == 4.0F);
    REQUIRE(s.data[15] == 1.0F);
}

TEST_CASE("matRotateY 90 degrees rotates X → -Z", "[math][mat4]")
{
    Matrix4 r = matRotateY(toRadians(90.0F));
    // rotation matrix should preserve identity-column structure (last row/col)
    REQUIRE(r.data[15] == Approx(1.0F));
    // column 0 is where X basis lands
    REQUIRE(r.data[0] == Approx(0.0F).margin(1e-5F));
    REQUIRE(r.data[2] == Approx(-1.0F).margin(1e-5F));
}

TEST_CASE("matPerspective sets w-divide column", "[math][mat4]")
{
    Matrix4 p = matPerspective(toRadians(60.0F), 16.0F / 9.0F, 0.1F, 100.0F);
    // classic perspective: m[11] = -1 (projects z into w), m[15] = 0
    REQUIRE(p.data[11] == Approx(-1.0F));
    REQUIRE(p.data[15] == Approx(0.0F));
    // upper-left entries non-zero, scaled by 1/(aspect*tan(fov/2))
    REQUIRE(p.data[0] > 0.0F);
    REQUIRE(p.data[5] > 0.0F);
}

TEST_CASE("matLookAt produces orthonormal basis", "[math][mat4]")
{
    Matrix4 v = matLookAt(Vec3 {0, 0, 5}, Vec3 {0, 0, 0}, Vec3 {0, 1, 0});
    // last row/col corner
    REQUIRE(v.data[15] == Approx(1.0F));
    // view matrix for an eye at (0,0,5) looking at origin translates z by -5
    REQUIRE(v.data[14] == Approx(-5.0F).margin(1e-4F));
}

TEST_CASE("matMultiply is associative for translations", "[math][mat4]")
{
    Matrix4 a = matTranslate({1, 2, 3});
    Matrix4 b = matTranslate({4, 5, 6});
    Matrix4 ab = a * b;
    // Combined translation column
    REQUIRE(ab.data[12] == Approx(5.0F));
    REQUIRE(ab.data[13] == Approx(7.0F));
    REQUIRE(ab.data[14] == Approx(9.0F));
}
