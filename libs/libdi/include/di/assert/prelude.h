#pragma once

#include <assert.h>
#include <di/container/string/fixed_string.h>
#include <di/util/compile_time_fail.h>
#include <di/util/source_location.h>

#define DI_ASSERT(...)                                                                         \
    do {                                                                                       \
        if (!(__VA_ARGS__)) {                                                                  \
            if consteval {                                                                     \
                di::util::compile_time_fail<di::container::FixedString { "" #__VA_ARGS__ }>(); \
            } else {                                                                           \
                assert(false);                                                                 \
            }                                                                                  \
        }                                                                                      \
    } while (0)

#define DI_ASSERT_EQ(a, b)     DI_ASSERT((a) == (b))
#define DI_ASSERT_NOT_EQ(a, b) DI_ASSERT((a) != (b))
#define DI_ASSERT_LT(a, b)     DI_ASSERT((a) < (b))
#define DI_ASSERT_LT_EQ(a, b)  DI_ASSERT((a) <= (b))
#define DI_ASSERT_GT(a, b)     DI_ASSERT((a) > (b))
#define DI_ASSERT_GT_EQ(a, b)  DI_ASSERT((a) >= (b))
