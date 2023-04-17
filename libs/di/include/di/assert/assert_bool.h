#pragma once

#include <di/assert/assert_interface.h>
#include <di/util/compile_time_fail.h>
#include <di/util/source_location.h>

#define DI_ASSERT(...)                                          \
    do {                                                        \
        if (!bool(__VA_ARGS__)) {                               \
            ::di::assert::detail::assert_fail("" #__VA_ARGS__); \
        }                                                       \
    } while (0)
