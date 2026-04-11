#include <catch2/catch_test_macros.hpp>

#include <engine/core/log.h>

#include <cstdio>
#include <cstring>

// shortFile is an inline helper in log.h. These tests exercise its path
// shortening logic — everything else in log.h is macros that write to
// stdout/stderr, not easily capturable in a unit test.

using namespace chad;

TEST_CASE("shortFile strips leading path up to chad-engine/", "[log]")
{
    const char *full = "/home/user/Repos/chad-engine/engine/src/platform/window.cpp";
    const char *s    = shortFile(full);
    REQUIRE(s != nullptr);
    REQUIRE(std::strstr(s, "engine/") != nullptr);
    REQUIRE(std::strstr(s, "window.cpp") != nullptr);
    REQUIRE(std::strstr(s, "src/platform/") == nullptr);  // path flattened
}

TEST_CASE("shortFile detects game module", "[log]")
{
    const char *full = "/home/user/Repos/chad-engine/game/src/main.cpp";
    const char *s    = shortFile(full);
    REQUIRE(std::strstr(s, "game/") != nullptr);
    REQUIRE(std::strstr(s, "main.cpp") != nullptr);
}

TEST_CASE("shortFile falls back when path does not contain project", "[log]")
{
    const char *full = "/some/unrelated/path/file.cpp";
    const char *s    = shortFile(full);
    // returns the original pointer (or similar) — just check it's non-null
    // and contains file.cpp
    REQUIRE(s != nullptr);
    REQUIRE(std::strstr(s, "file.cpp") != nullptr);
}

TEST_CASE("LOG_INFO macro expands and runs without crashing", "[log]")
{
    // Redirect stdout to /dev/null just for this call to keep test output clean.
    // If redirection fails we still run the log — it just leaks to stdout.
    FILE *saved = stdout;
    FILE *dev_null = std::fopen("/dev/null", "w");
    if (dev_null != nullptr) {
        stdout = dev_null;
    }

    LOG_INFO("test message: %d %s", 42, "hello");

    if (dev_null != nullptr) {
        stdout = saved;
        std::fclose(dev_null);
    }
    SUCCEED("LOG_INFO expanded and ran");
}
