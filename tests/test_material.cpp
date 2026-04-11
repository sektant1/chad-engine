#include <catch2/catch_test_macros.hpp>

#include <engine/renderer/material.h>
#include <engine/renderer/shader.h>
#include <engine/renderer/texture.h>

using namespace chad;

// Shader exposing the uniforms materialBind writes to. Real materials
// share this contract with their shader.
static const char *MAT_VERT = R"(
#version 330 core
layout(location = 0) in vec3 a_pos;
void main() { gl_Position = vec4(a_pos, 1.0); }
)";

static const char *MAT_FRAG = R"(
#version 330 core
uniform sampler2D uTexture;
uniform vec4 uTintColor;
uniform int uUseTexture;
out vec4 frag;
void main() {
    vec4 base = (uUseTexture == 1) ? texture(uTexture, vec2(0.0)) : vec4(1.0);
    frag = base * uTintColor;
}
)";

TEST_CASE("materialCreate references shader + texture without owning them", "[material]")
{
    Shader  s = shaderCreate(MAT_VERT, MAT_FRAG);
    Texture t = textureCreateWhite();
    REQUIRE(s.program != 0);
    REQUIRE(t.id != 0);

    Material m = materialCreate(&s, &t);
    REQUIRE(m.shader == &s);
    REQUIRE(m.texture == &t);
    REQUIRE(m.use_texture == true);
    REQUIRE(m.color.x == 1.0F);
    REQUIRE(m.color.w == 1.0F);

    shaderDestroy(s);
    textureDestroy(t);
}

TEST_CASE("materialCreateColored has null texture + untextured flag", "[material]")
{
    Shader s = shaderCreate(MAT_VERT, MAT_FRAG);
    REQUIRE(s.program != 0);

    Material m = materialCreateColored(&s, Vec4 {0.5F, 0.25F, 0.75F, 1.0F});
    REQUIRE(m.shader == &s);
    REQUIRE(m.texture == nullptr);
    REQUIRE(m.use_texture == false);
    REQUIRE(m.color.x == 0.5F);
    REQUIRE(m.color.y == 0.25F);
    REQUIRE(m.color.z == 0.75F);

    shaderDestroy(s);
}

TEST_CASE("materialBind with valid shader + texture sets uniforms", "[material]")
{
    Shader  s = shaderCreate(MAT_VERT, MAT_FRAG);
    Texture t = textureCreateWhite();
    REQUIRE(s.program != 0);

    Material m = materialCreate(&s, &t);
    m.color = {0.8F, 0.1F, 0.2F, 1.0F};

    materialBind(m);

    // After binding, shader cache should contain the three uniforms the
    // material writes to.
    REQUIRE(s.uniform_cache.count("uTexture") == 1);
    REQUIRE(s.uniform_cache.count("uUseTexture") == 1);
    REQUIRE(s.uniform_cache.count("uTintColor") == 1);

    materialUnbind();

    shaderDestroy(s);
    textureDestroy(t);
}

TEST_CASE("materialBind with use_texture=false skips texture sampler", "[material]")
{
    Shader s = shaderCreate(MAT_VERT, MAT_FRAG);
    REQUIRE(s.program != 0);

    Material m = materialCreateColored(&s, Vec4 {1, 0, 0, 1});
    materialBind(m);

    // Only tint + use-texture flag should be set (no sampler binding)
    REQUIRE(s.uniform_cache.count("uUseTexture") == 1);
    REQUIRE(s.uniform_cache.count("uTintColor") == 1);
    // uTexture sampler slot not written in the colored path
    REQUIRE(s.uniform_cache.count("uTexture") == 0);

    materialUnbind();
    shaderDestroy(s);
}

TEST_CASE("materialBind with null shader is a no-op", "[material]")
{
    Material m {};
    m.shader = nullptr;
    // must not crash — materialBind warns and returns
    materialBind(m);
    materialUnbind();
    SUCCEED("null shader handled");
}
