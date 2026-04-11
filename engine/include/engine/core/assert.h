#pragma once

#include <cstdio>
#include <cstdlib>
#include "log.h"

#define CHAD_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            LOG_FATAL("[ASSERT]: %s", #expr); \
        } \
    } while (0)

#define CHAD_ASSERT_MSG(expr, msg) \
    do { \
        if (!(expr)) { \
            LOG_FATAL("[ASSERT]: %s — %s", #expr, msg); \
        } \
    } while (0)
