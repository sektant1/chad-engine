#pragma once

#include <engine/core/types.h>
#include <engine/core/math.h>

namespace chad
{

struct Camera
{
    Vec3 position    = {0.0F, 1.0F, 3.0F};
    f32  yaw         = -90.0F;  // degrees, -90 = looking along -Z
    f32  pitch       = 0.0F;    // degrees
    f32  fov         = 90.0F;   // degrees, wide like Quake
    f32  near_plane  = 0.1F;
    f32  far_plane   = 50.0F;
    f32  sensitivity = 0.15F;
    f32  speed       = 5.0F;
};

Vec3 cameraForward(const Camera &cam);
Vec3 cameraRight(const Camera &cam);
Vec3 cameraUp(const Camera &cam);

Matrix4 cameraViewMatrix(const Camera &cam);
Matrix4 cameraProjectionMatrix(const Camera &cam, f32 aspect);

// FPS-style: WASD movement + mouse look
void cameraProcessMouse(Camera &cam, f32 dx, f32 dy);
void cameraProcessMovement(Camera &cam, f32 forward, f32 right, f32 dt);

}  // namespace chad
