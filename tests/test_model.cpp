#include <catch2/catch_test_macros.hpp>

#include <engine/renderer/model.h>

using namespace chad;

// These tests require a GL context (chad_gl_tests binary).
// They also need model assets in assets/models/.

static const char *TEST_GLB = "assets/models/bigfoot/glb/bigfoot.glb";
static const char *TEST_FBX = "assets/models/knight/knight.fbx";
static const char *TEST_OBJ = "assets/models/Biblically_Accurate_Angel/Biblically_Accurate_Angel.obj";

// ---------------- loading ----------------

TEST_CASE("modelLoad GLB produces valid model", "[model]")
{
    Model m = modelLoad(TEST_GLB);
    REQUIRE(m.mesh_count > 0);
    REQUIRE(m.mesh_count <= MODEL_MAX_MESHES);

    for (u32 i = 0; i < m.mesh_count; i++) {
        REQUIRE(m.meshes[i].mesh.vao != 0);
        REQUIRE(m.meshes[i].mesh.index_count > 0);
    }

    modelDestroy(m);
}

TEST_CASE("modelLoad FBX produces valid model", "[model]")
{
    Model m = modelLoad(TEST_FBX);
    REQUIRE(m.mesh_count > 0);

    for (u32 i = 0; i < m.mesh_count; i++) {
        REQUIRE(m.meshes[i].mesh.vao != 0);
    }

    modelDestroy(m);
}

TEST_CASE("modelLoad OBJ produces valid model", "[model]")
{
    Model m = modelLoad(TEST_OBJ);
    REQUIRE(m.mesh_count > 0);

    for (u32 i = 0; i < m.mesh_count; i++) {
        REQUIRE(m.meshes[i].mesh.vao != 0);
    }

    modelDestroy(m);
}

TEST_CASE("modelLoad invalid path returns empty model", "[model]")
{
    Model m = modelLoad("nonexistent/path/model.glb");
    REQUIRE(m.mesh_count == 0);
}

// ---------------- destroy ----------------

TEST_CASE("modelDestroy zeroes mesh_count", "[model]")
{
    Model m = modelLoad(TEST_GLB);
    REQUIRE(m.mesh_count > 0);

    modelDestroy(m);
    REQUIRE(m.mesh_count == 0);
}

TEST_CASE("modelDestroy on empty model is safe", "[model]")
{
    Model m = {};
    m.mesh_count = 0;
    modelDestroy(m);  // should not crash
    REQUIRE(m.mesh_count == 0);
}

// ---------------- draw ----------------

TEST_CASE("modelDraw does not crash on valid model", "[model]")
{
    Model m = modelLoad(TEST_GLB);
    REQUIRE(m.mesh_count > 0);

    // No shader bound — GL draw call still valid, just undefined output.
    modelDraw(m);
    modelDestroy(m);
}

TEST_CASE("modelDraw on empty model is safe", "[model]")
{
    Model m = {};
    m.mesh_count = 0;
    modelDraw(m);  // should not crash
}
