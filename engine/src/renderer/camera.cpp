
#include <engine/renderer/camera.h>

namespace chad
{

Vec3 cameraForward(const Camera &cam)
{
    f32 yaw_rad   = toRadians(cam.yaw);
    f32 pitch_rad = toRadians(cam.pitch);
    return vec3Normalize({cosf(yaw_rad) * cosf(pitch_rad), sinf(pitch_rad), sinf(yaw_rad) * cosf(pitch_rad)});
}

Vec3 cameraRight(const Camera &cam)
{
    Vec3 fwd = cameraForward(cam);
    return vec3Normalize(vec3Cross(fwd, {0.0F, 1.0F, 0.0F}));
}

Vec3 cameraUp(const Camera &cam)
{
    Vec3 fwd   = cameraForward(cam);
    Vec3 right = cameraRight(cam);
    return vec3Cross(right, fwd);
}

Matrix4 cameraViewMatrix(const Camera &cam)
{
    Vec3 fwd    = cameraForward(cam);
    Vec3 target = cam.position + fwd;
    return matLookAt(cam.position, target, {0.0F, 1.0F, 0.0F});
}

Matrix4 cameraProjectionMatrix(const Camera &cam, f32 aspect)
{
    return matPerspective(toRadians(cam.fov), aspect, cam.near_plane, cam.far_plane);
}

void cameraProcessMouse(Camera &cam, f32 dx, f32 dy)
{
    cam.yaw += dx * cam.sensitivity;
    cam.pitch -= dy * cam.sensitivity;
    cam.pitch = clampf(cam.pitch, -89.0F, 89.0F);
}

void cameraProcessMovement(Camera &cam, f32 forward, f32 right, f32 dt)
{
    Vec3 fwd = cameraForward(cam);
    // FPS: project forward onto XZ plane for horizontal movement
    Vec3 fwd_flat  = vec3Normalize({fwd.x, 0.0F, fwd.z});
    Vec3 right_dir = cameraRight(cam);

    Vec3 move = (fwd_flat * forward) + (right_dir * right);

    // Clamp magnitude to 1 so diagonals don't speed up, but preserve analog input below 1.
    if (vec3LenSquare(move) > 1.0F) {
        move = vec3Normalize(move);
    }

    cam.position += move * (cam.speed * dt);
}

}  // namespace chad
