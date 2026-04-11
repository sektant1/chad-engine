#pragma once

#include <engine/core/types.h>

namespace chad
{

struct Vertex
{
    f32 position[3];
    f32 texcoord[2];
    f32 normal[3];
    f32 color[4];
};

struct Mesh
{
    u32 vao;
    u32 vbo;
    u32 ebo;
    u32 index_count;
};

Mesh meshCreate(const Vertex *vertices, u32 vert_count, const u32 *indices, u32 idx_count);
void meshDestroy(Mesh &mesh);
void meshDraw(const Mesh &mesh);

// Helpers for quick testing
Mesh meshCreateTriangle();
Mesh meshCreateCube();

}  // namespace chad
