#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <engine/renderer/camera.h>

#include <cmath>

using namespace chad;
using Catch::Approx;

// ---------------- defaults ----------------

TEST_CASE("Camera default values", "[camera]")
{
    Camera cam {};
    REQUIRE(cam.position.x == 0.0F);
    REQUIRE(cam.position.y == 1.0F);
    REQUIRE(cam.position.z == 3.0F);
    REQUIRE(cam.yaw == -90.0F);
    REQUIRE(cam.pitch == 0.0F);
    REQUIRE(cam.fov == 90.0F);
    REQUIRE(cam.near_plane == Approx(0.1F));
    REQUIRE(cam.far_plane == Approx(50.0F));
}

// ---------------- forward / right / up ----------------

TEST_CASE("cameraForward at default yaw/pitch points along -Z", "[camera]")
{
    Camera cam {};
    Vec3   fwd = cameraForward(cam);
    REQUIRE(fwd.x == Approx(0.0F).margin(1e-5F));
    REQUIRE(fwd.y == Approx(0.0F).margin(1e-5F));
    REQUIRE(fwd.z == Approx(-1.0F).margin(1e-5F));
}

TEST_CASE("cameraForward is unit length", "[camera]")
{
    Camera cam {};
    cam.yaw   = 45.0F;
    cam.pitch = 30.0F;
    Vec3 fwd  = cameraForward(cam);
    REQUIRE(vec3Length(fwd) == Approx(1.0F).margin(1e-5F));
}

TEST_CASE("cameraForward at yaw=0 pitch=0 points along +X", "[camera]")
{
    Camera cam {};
    cam.yaw   = 0.0F;
    cam.pitch = 0.0F;
    Vec3 fwd  = cameraForward(cam);
    REQUIRE(fwd.x == Approx(1.0F).margin(1e-5F));
    REQUIRE(fwd.y == Approx(0.0F).margin(1e-5F));
    REQUIRE(fwd.z == Approx(0.0F).margin(1e-5F));
}

TEST_CASE("cameraRight is perpendicular to forward", "[camera]")
{
    Camera cam {};
    cam.yaw   = 30.0F;
    cam.pitch = 15.0F;
    Vec3 fwd   = cameraForward(cam);
    Vec3 right = cameraRight(cam);
    REQUIRE(vec3Dot(fwd, right) == Approx(0.0F).margin(1e-4F));
}

TEST_CASE("cameraRight is unit length", "[camera]")
{
    Camera cam {};
    cam.yaw = 60.0F;
    Vec3 right = cameraRight(cam);
    REQUIRE(vec3Length(right) == Approx(1.0F).margin(1e-5F));
}

TEST_CASE("cameraUp is perpendicular to forward and right", "[camera]")
{
    Camera cam {};
    cam.yaw   = -45.0F;
    cam.pitch = 20.0F;
    Vec3 fwd   = cameraForward(cam);
    Vec3 right = cameraRight(cam);
    Vec3 up    = cameraUp(cam);
    REQUIRE(vec3Dot(fwd, up) == Approx(0.0F).margin(1e-4F));
    REQUIRE(vec3Dot(right, up) == Approx(0.0F).margin(1e-4F));
}

// ---------------- view matrix ----------------

TEST_CASE("cameraViewMatrix is valid 4x4 transform", "[camera]")
{
    Camera cam {};
    Matrix4 view = cameraViewMatrix(cam);
    // last row/col corner = 1
    REQUIRE(view.data[15] == Approx(1.0F));
    // translation component reflects camera position
    // view matrix translates world by -eye along view axes
    REQUIRE(view.data[14] == Approx(-3.0F).margin(1e-3F));
}

TEST_CASE("cameraViewMatrix at origin looking at -Z matches matLookAt", "[camera]")
{
    Camera cam {};
    cam.position = {0.0F, 0.0F, 0.0F};
    cam.yaw      = -90.0F;
    cam.pitch    = 0.0F;

    Matrix4 view = cameraViewMatrix(cam);
    Matrix4 expected = matLookAt({0, 0, 0}, {0, 0, -1}, {0, 1, 0});

    for (int i = 0; i < 16; ++i) {
        REQUIRE(view.data[i] == Approx(expected.data[i]).margin(1e-4F));
    }
}

// ---------------- projection ----------------

TEST_CASE("cameraProjectionMatrix has perspective w-divide", "[camera]")
{
    Camera cam {};
    Matrix4 proj = cameraProjectionMatrix(cam, 16.0F / 9.0F);
    // m[11] = -1 (perspective divide)
    REQUIRE(proj.data[11] == Approx(-1.0F));
    REQUIRE(proj.data[15] == Approx(0.0F));
    REQUIRE(proj.data[0] > 0.0F);
    REQUIRE(proj.data[5] > 0.0F);
}

TEST_CASE("cameraProjectionMatrix matches matPerspective", "[camera]")
{
    Camera cam {};
    f32 aspect = 1920.0F / 1080.0F;
    Matrix4 proj = cameraProjectionMatrix(cam, aspect);
    Matrix4 expected = matPerspective(toRadians(cam.fov), aspect, cam.near_plane, cam.far_plane);

    for (int i = 0; i < 16; ++i) {
        REQUIRE(proj.data[i] == Approx(expected.data[i]).margin(1e-5F));
    }
}

// ---------------- mouse look ----------------

TEST_CASE("cameraProcessMouse applies sensitivity to yaw/pitch", "[camera]")
{
    Camera cam {};
    f32 orig_yaw   = cam.yaw;
    f32 orig_pitch = cam.pitch;

    cameraProcessMouse(cam, 100.0F, 50.0F);

    REQUIRE(cam.yaw == Approx(orig_yaw + 100.0F * cam.sensitivity));
    // pitch inverted (dy subtracted)
    REQUIRE(cam.pitch == Approx(orig_pitch - 50.0F * cam.sensitivity));
}

TEST_CASE("cameraProcessMouse clamps pitch to [-89, 89]", "[camera]")
{
    Camera cam {};

    // huge upward look → clamp at 89
    cameraProcessMouse(cam, 0.0F, -100000.0F);
    REQUIRE(cam.pitch == Approx(89.0F));

    // huge downward look → clamp at -89
    cam.pitch = 0.0F;
    cameraProcessMouse(cam, 0.0F, 100000.0F);
    REQUIRE(cam.pitch == Approx(-89.0F));
}

TEST_CASE("cameraProcessMouse zero input changes nothing", "[camera]")
{
    Camera cam {};
    f32 yaw   = cam.yaw;
    f32 pitch = cam.pitch;

    cameraProcessMouse(cam, 0.0F, 0.0F);
    REQUIRE(cam.yaw == yaw);
    REQUIRE(cam.pitch == pitch);
}

// ---------------- movement ----------------

TEST_CASE("cameraProcessMovement forward moves along fwd_flat (XZ plane)", "[camera]")
{
    Camera cam {};
    cam.position = {0.0F, 0.0F, 0.0F};
    // default yaw=-90 → forward is -Z
    cameraProcessMovement(cam, 1.0F, 0.0F, 1.0F);

    REQUIRE(cam.position.x == Approx(0.0F).margin(1e-4F));
    REQUIRE(cam.position.y == Approx(0.0F).margin(1e-4F));
    REQUIRE(cam.position.z == Approx(-cam.speed).margin(1e-4F));
}

TEST_CASE("cameraProcessMovement right moves along right dir", "[camera]")
{
    Camera cam {};
    cam.position = {0.0F, 0.0F, 0.0F};
    // default → right is +X
    cameraProcessMovement(cam, 0.0F, 1.0F, 1.0F);

    REQUIRE(cam.position.x == Approx(cam.speed).margin(1e-4F));
    REQUIRE(cam.position.y == Approx(0.0F).margin(1e-4F));
    REQUIRE(cam.position.z == Approx(0.0F).margin(1e-4F));
}

TEST_CASE("cameraProcessMovement diagonal does not exceed speed", "[camera]")
{
    Camera cam {};
    cam.position = {0.0F, 0.0F, 0.0F};
    cameraProcessMovement(cam, 1.0F, 1.0F, 1.0F);

    f32 dist = vec3Length(cam.position);
    // diagonal clamped to 1 → dist = speed * dt
    REQUIRE(dist == Approx(cam.speed).margin(1e-3F));
}

TEST_CASE("cameraProcessMovement zero input stays put", "[camera]")
{
    Camera cam {};
    Vec3 orig = cam.position;
    cameraProcessMovement(cam, 0.0F, 0.0F, 1.0F);

    REQUIRE(cam.position.x == orig.x);
    REQUIRE(cam.position.y == orig.y);
    REQUIRE(cam.position.z == orig.z);
}

TEST_CASE("cameraProcessMovement respects dt scaling", "[camera]")
{
    Camera cam {};
    cam.position = {0.0F, 0.0F, 0.0F};
    cameraProcessMovement(cam, 1.0F, 0.0F, 0.5F);

    f32 dist = vec3Length(cam.position);
    REQUIRE(dist == Approx(cam.speed * 0.5F).margin(1e-4F));
}

TEST_CASE("cameraProcessMovement Y stays constant (FPS style)", "[camera]")
{
    Camera cam {};
    cam.position = {0.0F, 5.0F, 0.0F};
    cam.pitch    = 45.0F;  // looking up
    cameraProcessMovement(cam, 1.0F, 0.0F, 1.0F);

    // FPS: forward projected to XZ plane → Y unchanged
    REQUIRE(cam.position.y == Approx(5.0F).margin(1e-4F));
}
