#include <catch2/catch_test_macros.hpp>

#include <engine/core/types.h>

#include <type_traits>

using namespace chad;

// These are compile-time guarantees. Runtime assertions just surface them
// in the test report — if the static_asserts fire, the file doesn't compile.

static_assert(sizeof(u8) == 1, "u8 must be 1 byte");
static_assert(sizeof(u16) == 2, "u16 must be 2 bytes");
static_assert(sizeof(u32) == 4, "u32 must be 4 bytes");
static_assert(sizeof(u64) == 8, "u64 must be 8 bytes");

static_assert(sizeof(i8) == 1, "i8 must be 1 byte");
static_assert(sizeof(i16) == 2, "i16 must be 2 bytes");
static_assert(sizeof(i32) == 4, "i32 must be 4 bytes");
static_assert(sizeof(i64) == 8, "i64 must be 8 bytes");

static_assert(sizeof(f32) == 4, "f32 must be 4 bytes");
static_assert(sizeof(f64) == 8, "f64 must be 8 bytes");

static_assert(std::is_unsigned_v<u8>, "u8 must be unsigned");
static_assert(std::is_unsigned_v<u32>, "u32 must be unsigned");
static_assert(std::is_signed_v<i32>, "i32 must be signed");
static_assert(std::is_floating_point_v<f32>, "f32 must be floating point");
static_assert(std::is_floating_point_v<f64>, "f64 must be floating point");

TEST_CASE("fixed-width types match expected widths", "[types]")
{
    REQUIRE(sizeof(u8)  == 1);
    REQUIRE(sizeof(u16) == 2);
    REQUIRE(sizeof(u32) == 4);
    REQUIRE(sizeof(u64) == 8);

    REQUIRE(sizeof(i8)  == 1);
    REQUIRE(sizeof(i16) == 2);
    REQUIRE(sizeof(i32) == 4);
    REQUIRE(sizeof(i64) == 8);

    REQUIRE(sizeof(f32) == 4);
    REQUIRE(sizeof(f64) == 8);
}

TEST_CASE("usize matches platform pointer width", "[types]")
{
    REQUIRE(sizeof(usize) == sizeof(void *));
}
