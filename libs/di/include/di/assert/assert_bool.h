#pragma once

#include "di/assert/assert_interface.h"
#include "di/util/compile_time_fail.h"
#include "di/util/source_location.h"

#define DI_ASSERT(...)                                                                \
    do {                                                                              \
        if (!bool(__VA_ARGS__)) {                                                     \
            ::di::assert::detail::assert_fail("" #__VA_ARGS__, nullptr, nullptr,      \
                                              ::di::util::SourceLocation::current()); \
        }                                                                             \
    } while (0)

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_ASSERT)
#define ASSERT DI_ASSERT
#endif
