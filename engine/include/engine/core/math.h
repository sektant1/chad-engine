#pragma once

#include <array>
#include <cmath>
#include "types.h"

namespace chad
{

// ---- Utility ----
constexpr f32 CHAD_PI = 3.14159265358979323846F;

inline f32 toRadians(f32 degrees)
{
    return (CHAD_PI / 180.0F) * degrees;
}

inline f32 toDegrees(f32 radians)
{
    return (180.0F / CHAD_PI) * radians;
}

inline f32 clampf(f32 value, f32 min, f32 max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

struct Vec2
{
    f32 x;
    f32 y;
};

inline Vec2 vec2(f32 x, f32 y)
{
    return {x, y};
}

inline Vec2 operator+(Vec2 lhs, Vec2 rhs)
{
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

inline Vec2 operator-(Vec2 lhs, Vec2 rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

inline Vec2 operator*(Vec2 lhs, f32 scalar)
{
    return {lhs.x * scalar, lhs.y * scalar};
}

inline Vec2 operator*(f32 scalar, Vec2 rhs)
{
    return {rhs.x * scalar, rhs.y * scalar};
}

struct Vec3
{
    f32 x;
    f32 y;
    f32 z;
};

inline Vec3 vec3(f32 x, f32 y, f32 z)
{
    return {x, y, z};
}

inline Vec3 vec3(f32 scalar)
{
    return {scalar, scalar, scalar};
}

inline Vec3 operator+(Vec3 lhs, Vec3 rhs)
{
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

inline Vec3 operator-(Vec3 lhs, Vec3 rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

inline Vec3 operator*(Vec3 lhs, f32 scalar)
{
    return {lhs.x * scalar, lhs.y * scalar, lhs.z * scalar};
}

inline Vec3 operator*(f32 scalar, Vec3 rhs)
{
    return {rhs.x * scalar, rhs.y * scalar, rhs.z * scalar};
}

inline Vec3 operator-(Vec3 rhs)
{
    return {-rhs.x, -rhs.y, -rhs.z};
}

inline Vec3 &operator+=(Vec3 &lhs, Vec3 rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;
    return lhs;
}

inline Vec3 &operator-=(Vec3 &lhs, Vec3 rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    lhs.z -= rhs.z;
    return lhs;
}

inline Vec3 &operator*=(Vec3 &lhs, f32 scalar)
{
    lhs.x *= scalar;
    lhs.y *= scalar;
    lhs.z *= scalar;
    return lhs;
}

inline f32 vec3Dot(Vec3 lhs, Vec3 rhs)
{
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

inline f32 vec3LenSquare(Vec3 rhs)
{
    return vec3Dot(rhs, rhs);
}

inline f32 vec3Length(Vec3 rhs)
{
    return sqrtf(vec3LenSquare(rhs));
}

inline Vec3 vec3Cross(Vec3 lhs, Vec3 rhs)
{
    return {(lhs.y * rhs.z) - (lhs.z * rhs.y), (lhs.z * rhs.x) - (lhs.x * rhs.z), (lhs.x * rhs.y) - (lhs.y * rhs.x)};
}

inline Vec3 vec3Normalize(Vec3 rhs)
{
    f32 length = vec3Length(rhs);
    if (length < 1e-8F) {
        return {0, 0, 0};
    }
    f32 inverse = 1.0F / length;
    return {rhs.x * inverse, rhs.y * inverse, rhs.z * inverse};
}

inline Vec3 vec3Lerp(Vec3 lhs, Vec3 rhs, f32 time)
{
    return lhs + (rhs - lhs) * time;
}

struct Vec4
{
    f32 x;
    f32 y;
    f32 z;
    f32 w;
};

inline Vec4 vec4(f32 x, f32 y, f32 z, f32 w)
{
    return {x, y, z, w};
}

inline Vec4 vec4(Vec3 vec, f32 w)
{
    return {vec.x, vec.y, vec.z, w};
}

// Mat4 (column-major, OpenGL convention)
// m[col][row] stored as flat array: data[col*4 + row]
struct Matrix4
{
    std::array<f32, 16> data;

    f32 &at(i32 col, i32 row) { return data[row + (col * 4)]; }

    [[nodiscard]] f32 at(i32 col, i32 row) const { return data[row + (col * 4)]; }
};

inline Matrix4 matIdentity()
{
    Matrix4 result  = {};
    result.data[0]  = 1.0F;
    result.data[5]  = 1.0F;
    result.data[10] = 1.0F;
    result.data[15] = 1.0F;
    return result;
}

inline Matrix4 matMultiply(const Matrix4 &lhs, const Matrix4 &rhs)
{
    Matrix4 result = {};
    for (i32 col = 0; col < 4; col++) {
        for (i32 row = 0; row < 4; row++) {
            f32 sum = 0.0F;
            for (i32 k = 0; k < 4; k++) {
                sum += lhs.data[(k * 4) + row] * rhs.data[(col * 4) + k];
            }
            result.data[(col * 4) + row] = sum;
        }
    }
    return result;
}

inline Matrix4 operator*(const Matrix4 &lhs, const Matrix4 &rhs)
{
    return matMultiply(lhs, rhs);
}

inline Matrix4 matTranslate(Vec3 vec_t)
{
    Matrix4 result  = matIdentity();
    result.data[12] = vec_t.x;
    result.data[13] = vec_t.y;
    result.data[14] = vec_t.z;
    return result;
}

inline Matrix4 matScale(Vec3 scalar)
{
    Matrix4 result  = {};
    result.data[0]  = scalar.x;
    result.data[5]  = scalar.y;
    result.data[10] = scalar.z;
    result.data[15] = 1.0f;
    return result;
}

inline Matrix4 matRotateX(f32 radians)
{
    f32 cos = cosf(radians);
    f32 sin = sinf(radians);

    Matrix4 result  = matIdentity();
    result.data[5]  = cos;
    result.data[9]  = -sin;
    result.data[6]  = sin;
    result.data[10] = cos;
    return result;
}

inline Matrix4 matRotateY(f32 radians)
{
    f32 cos = cosf(radians);
    f32 sin = sinf(radians);

    Matrix4 result  = matIdentity();
    result.data[0]  = cos;
    result.data[8]  = sin;
    result.data[2]  = -sin;
    result.data[10] = cos;
    return result;
}

inline Matrix4 matRotateZ(f32 radians)
{
    f32 cos = cosf(radians);
    f32 sin = sinf(radians);

    Matrix4 result = matIdentity();
    result.data[0] = cos;
    result.data[4] = -sin;
    result.data[1] = sin;
    result.data[5] = cos;
    return result;
}

inline Matrix4 matPerspective(f32 fov_radians, f32 aspect, f32 near, f32 far)
{
    f32     tan_half = tanf(fov_radians * 0.5F);
    Matrix4 result   = {};
    result.data[0]   = 1.0F / (aspect * tan_half);
    result.data[5]   = 1.0F / tan_half;
    result.data[10]  = -(near + far) / (far - near);
    result.data[11]  = -1.0F;
    result.data[14]  = -(far * 2.0F * near) / (far - near);
    return result;
}

inline Matrix4 matLookAt(Vec3 eye, Vec3 center, Vec3 up)
{
    Vec3 forward = vec3Normalize(center - eye);
    Vec3 right   = vec3Normalize(vec3Cross(forward, up));
    Vec3 true_up = vec3Cross(right, forward);

    Matrix4 result  = matIdentity();
    result.data[0]  = right.x;
    result.data[4]  = right.y;
    result.data[8]  = right.z;
    result.data[1]  = true_up.x;
    result.data[5]  = true_up.y;
    result.data[9]  = true_up.z;
    result.data[2]  = -forward.x;
    result.data[6]  = -forward.y;
    result.data[10] = -forward.z;
    result.data[12] = -vec3Dot(right, eye);
    result.data[13] = -vec3Dot(true_up, eye);
    result.data[14] = vec3Dot(forward, eye);
    return result;
}

}  // namespace chad
