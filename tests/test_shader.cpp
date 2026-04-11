#include <catch2/catch_test_macros.hpp>

#include <engine/renderer/shader.h>
#include <engine/core/math.h>

using namespace chad;

static const char *VALID_VERT = R"(
#version 330 core
layout(location = 0) in vec3 a_pos;
uniform mat4 u_mvp;
void main() { gl_Position = u_mvp * vec4(a_pos, 1.0); }
)";

static const char *VALID_FRAG = R"(
#version 330 core
uniform vec3 u_color;
uniform float u_intensity;
out vec4 frag;
void main() { frag = vec4(u_color * u_intensity, 1.0); }
)";

static const char *BROKEN_VERT = R"(
#version 330 core
this is not valid glsl
)";

TEST_CASE("shaderCreate compiles + links valid shaders", "[shader]")
{
    Shader s = shaderCreate(VALID_VERT, VALID_FRAG);
    REQUIRE(s.program != 0);
    shaderDestroy(s);
    REQUIRE(s.program == 0);
}

TEST_CASE("shaderCreate returns empty shader on broken GLSL", "[shader]")
{
    Shader s = shaderCreate(BROKEN_VERT, VALID_FRAG);
    REQUIRE(s.program == 0);
    shaderDestroy(s);  // safe on empty shader
}

TEST_CASE("shaderLoad returns empty shader for missing files", "[shader]")
{
    Shader s = shaderLoad("nonexistent.vert", "nonexistent.frag");
    REQUIRE(s.program == 0);
}

TEST_CASE("uniform setters cache locations", "[shader]")
{
    Shader s = shaderCreate(VALID_VERT, VALID_FRAG);
    REQUIRE(s.program != 0);

    shaderBind(s);
    shaderSetVec3(s, "u_color", 1.0F, 0.5F, 0.25F);
    shaderSetFloat(s, "u_intensity", 0.9F);

    // cache should now hold both uniforms
    REQUIRE(s.uniform_cache.size() == 2);
    REQUIRE(s.uniform_cache.count("u_color") == 1);
    REQUIRE(s.uniform_cache.count("u_intensity") == 1);

    // repeat call should reuse cache (size unchanged)
    shaderSetFloat(s, "u_intensity", 0.5F);
    REQUIRE(s.uniform_cache.size() == 2);

    Matrix4 identity = matIdentity();
    shaderSetMatrix4(s, "u_mvp", identity);
    REQUIRE(s.uniform_cache.size() == 3);

    shaderUnbind();
    shaderDestroy(s);
    // destroy clears cache
    REQUIRE(s.uniform_cache.empty());
}

TEST_CASE("uniform setter warns on missing uniform but does not crash", "[shader]")
{
    Shader s = shaderCreate(VALID_VERT, VALID_FRAG);
    REQUIRE(s.program != 0);

    shaderBind(s);
    shaderSetFloat(s, "does_not_exist", 1.0F);
    // location cached as -1
    REQUIRE(s.uniform_cache.at("does_not_exist") == -1);

    shaderDestroy(s);
}
