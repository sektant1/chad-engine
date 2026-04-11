#include <catch2/catch_test_macros.hpp>

#include <engine/core/assert.h>

// CHAD_ASSERT calls abort() on failure — we can't test the failure path in
// a normal unit test without killing the process. Death-test support is
// not enabled here, so we only verify the "passing condition" path:
// truthy conditions must not abort and must not alter program state.

TEST_CASE("CHAD_ASSERT passes on true condition", "[assert]")
{
    CHAD_ASSERT(1 + 1 == 2);
    CHAD_ASSERT(true);

    int x = 5;
    CHAD_ASSERT(x > 0);

    int *ptr = &x;
    CHAD_ASSERT(ptr != nullptr);

    SUCCEED("all truthy asserts passed");
}

TEST_CASE("CHAD_ASSERT_MSG passes on true condition", "[assert]")
{
    CHAD_ASSERT_MSG(true, "should never fire");
    CHAD_ASSERT_MSG(42 == 42, "identity");

    SUCCEED("all truthy asserts with message passed");
}

TEST_CASE("CHAD_ASSERT does not evaluate expression side-effects more than once", "[assert]")
{
    int counter = 0;
    CHAD_ASSERT(++counter == 1);
    REQUIRE(counter == 1);
}
