#include <catch2/catch_test_macros.hpp>

#include <engine/renderer/mesh.h>

using namespace chad;

TEST_CASE("meshCreate builds VAO/VBO/EBO with given counts", "[mesh]")
{
    Vertex verts[] = {
        {{-0.5F, -0.5F, 0.0F}, {0, 0}, {0, 0, 1}, {1, 0, 0, 1}},
        {{0.5F, -0.5F, 0.0F},  {1, 0}, {0, 0, 1}, {0, 1, 0, 1}},
        {{0.0F, 0.5F, 0.0F},   {0, 1}, {0, 0, 1}, {0, 0, 1, 1}},
    };
    u32 idx[] = {0, 1, 2};

    Mesh m = meshCreate(verts, 3, idx, 3);
    REQUIRE(m.vao != 0);
    REQUIRE(m.vbo != 0);
    REQUIRE(m.ebo != 0);
    REQUIRE(m.index_count == 3);

    meshDestroy(m);
    REQUIRE(m.vao == 0);
    REQUIRE(m.vbo == 0);
    REQUIRE(m.ebo == 0);
}

TEST_CASE("meshCreateTriangle returns a valid mesh", "[mesh]")
{
    Mesh m = meshCreateTriangle();
    REQUIRE(m.vao != 0);
    REQUIRE(m.index_count == 3);
    meshDestroy(m);
}

TEST_CASE("meshCreateCube has 36 indices (6 faces × 2 tris × 3 verts)", "[mesh]")
{
    Mesh m = meshCreateCube();
    REQUIRE(m.vao != 0);
    REQUIRE(m.index_count == 36);
    meshDestroy(m);
}

TEST_CASE("meshDraw does not crash on valid mesh", "[mesh]")
{
    Mesh m = meshCreateTriangle();
    // No shader bound — the draw call is still a valid GL command,
    // it just produces undefined output. This ensures the VAO/EBO state
    // is coherent and meshDraw doesn't error out on binding.
    meshDraw(m);
    meshDestroy(m);
}
