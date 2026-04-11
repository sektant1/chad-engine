#include <engine/renderer/mesh.h>

#include <glad/glad.h>

namespace chad
{

Mesh meshCreate(const Vertex *vertices, u32 vert_count, const u32 *indices, u32 idx_count)
{
    Mesh m        = {};
    m.index_count = idx_count;

    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glGenBuffers(1, &m.ebo);

    glBindVertexArray(m.vao);

    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, vert_count * sizeof(Vertex), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx_count * sizeof(u32), indices, GL_STATIC_DRAW);

    // position: location 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    // texcoord: location 1
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texcoord));
    glEnableVertexAttribArray(1);

    // normal: location 2
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);

    // color: location 3
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    return m;
}

void meshDestroy(Mesh &m)
{
    glDeleteVertexArrays(1, &m.vao);
    glDeleteBuffers(1, &m.vbo);
    glDeleteBuffers(1, &m.ebo);
    m = {};
}

void meshDraw(const Mesh &m)
{
    glBindVertexArray(m.vao);
    glDrawElements(GL_TRIANGLES, m.index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

Mesh meshCreateTriangle()
{
    Vertex vertices[] = {
        // position           texcoord   normal          color
        {{0.0f, 0.5f, 0.0f},   {0.5f, 1.0f}, {0, 0, 1}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}, {0, 0, 1}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.0f},  {1.0f, 0.0f}, {0, 0, 1}, {0.0f, 0.0f, 1.0f, 1.0f}},
    };
    u32 indices[] = {0, 1, 2};
    return meshCreate(vertices, 3, indices, 3);
}

Mesh meshCreateCube()
{
    Vertex vertices[] = {
        // Front face
        {{-0.5F, -0.5F, 0.5F},  {0, 0}, {0, 0, 1},  {1, 1, 1, 1}},
        {{0.5F, -0.5F, 0.5F},   {1, 0}, {0, 0, 1},  {1, 1, 1, 1}},
        {{0.5F, 0.5F, 0.5F},    {1, 1}, {0, 0, 1},  {1, 1, 1, 1}},
        {{-0.5F, 0.5F, 0.5F},   {0, 1}, {0, 0, 1},  {1, 1, 1, 1}},
        // Back face
        {{0.5F, -0.5F, -0.5F},  {0, 0}, {0, 0, -1}, {1, 1, 1, 1}},
        {{-0.5F, -0.5F, -0.5F}, {1, 0}, {0, 0, -1}, {1, 1, 1, 1}},
        {{-0.5F, 0.5F, -0.5F},  {1, 1}, {0, 0, -1}, {1, 1, 1, 1}},
        {{0.5F, 0.5F, -0.5F},   {0, 1}, {0, 0, -1}, {1, 1, 1, 1}},
        // Top face
        {{-0.5F, 0.5F, 0.5F},   {0, 0}, {0, 1, 0},  {1, 1, 1, 1}},
        {{0.5F, 0.5F, 0.5F},    {1, 0}, {0, 1, 0},  {1, 1, 1, 1}},
        {{0.5F, 0.5F, -0.5F},   {1, 1}, {0, 1, 0},  {1, 1, 1, 1}},
        {{-0.5F, 0.5F, -0.5F},  {0, 1}, {0, 1, 0},  {1, 1, 1, 1}},
        // Bottom face
        {{-0.5F, -0.5F, -0.5F}, {0, 0}, {0, -1, 0}, {1, 1, 1, 1}},
        {{0.5F, -0.5F, -0.5F},  {1, 0}, {0, -1, 0}, {1, 1, 1, 1}},
        {{0.5F, -0.5F, 0.5F},   {1, 1}, {0, -1, 0}, {1, 1, 1, 1}},
        {{-0.5F, -0.5F, 0.5F},  {0, 1}, {0, -1, 0}, {1, 1, 1, 1}},
        // Right face
        {{0.5F, -0.5F, 0.5F},   {0, 0}, {1, 0, 0},  {1, 1, 1, 1}},
        {{0.5F, -0.5F, -0.5F},  {1, 0}, {1, 0, 0},  {1, 1, 1, 1}},
        {{0.5F, 0.5F, -0.5F},   {1, 1}, {1, 0, 0},  {1, 1, 1, 1}},
        {{0.5F, 0.5F, 0.5F},    {0, 1}, {1, 0, 0},  {1, 1, 1, 1}},
        // Left face
        {{-0.5F, -0.5F, -0.5F}, {0, 0}, {-1, 0, 0}, {1, 1, 1, 1}},
        {{-0.5F, -0.5F, 0.5F},  {1, 0}, {-1, 0, 0}, {1, 1, 1, 1}},
        {{-0.5F, 0.5F, 0.5F},   {1, 1}, {-1, 0, 0}, {1, 1, 1, 1}},
        {{-0.5F, 0.5F, -0.5F},  {0, 1}, {-1, 0, 0}, {1, 1, 1, 1}},
    };

    u32 indices[] = {
        0,  1,  2,  2,  3,  0,   // front
        4,  5,  6,  6,  7,  4,   // back
        8,  9,  10, 10, 11, 8,   // top
        12, 13, 14, 14, 15, 12,  // bottom
        16, 17, 18, 18, 19, 16,  // right
        20, 21, 22, 22, 23, 20,  // left
    };

    return meshCreate(vertices, 24, indices, 36);
}

}  // namespace chad
