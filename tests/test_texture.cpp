#include <catch2/catch_test_macros.hpp>

#include <engine/renderer/texture.h>

using namespace chad;

TEST_CASE("textureCreateWhite produces a 1x1 RGBA texture", "[texture]")
{
    Texture t = textureCreateWhite();
    REQUIRE(t.id != 0);
    REQUIRE(t.width == 1);
    REQUIRE(t.height == 1);
    REQUIRE(t.channels == 4);
    textureDestroy(t);
    REQUIRE(t.id == 0);
}

TEST_CASE("textureLoad falls back to Brick_0 on missing file", "[texture]")
{
    // This path does not exist — textureLoad should fall back to
    // assets/textures/Brick_0.png (which does exist when cwd = repo root).
    Texture t = textureLoad("does_not_exist_xyz.png");
    REQUIRE(t.id != 0);
    // Fallback image has non-trivial dimensions
    REQUIRE(t.width > 1);
    REQUIRE(t.height > 1);
    textureDestroy(t);
}

TEST_CASE("textureLoad loads a real asset", "[texture]")
{
    Texture t = textureLoad("assets/textures/Brick_0.png");
    REQUIRE(t.id != 0);
    REQUIRE(t.width > 0);
    REQUIRE(t.height > 0);
    REQUIRE((t.channels == 3 || t.channels == 4));
    textureDestroy(t);
}

TEST_CASE("textureBind/Unbind on valid slots", "[texture]")
{
    Texture t = textureCreateWhite();
    textureBind(t, 0);
    textureBind(t, 3);
    textureUnbind(0);
    textureUnbind(3);
    textureDestroy(t);
}
