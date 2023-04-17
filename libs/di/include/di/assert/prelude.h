#pragma once

#include <di/concepts/equality_comparable_with.h>
#include <di/container/string/fixed_string.h>
#include <di/function/equal.h>
#include <di/function/equal_or_greater.h>
#include <di/function/equal_or_less.h>
#include <di/function/greater.h>
#include <di/function/less.h>
#include <di/function/not_equal.h>
#include <di/util/compile_time_fail.h>
#include <di/util/source_location.h>

namespace di::assert::detail {
[[noreturn]] inline void do_assert_fail(char const* message, util::SourceLocation loc);

template<typename T, typename U>
[[noreturn]] inline void do_binary_assert_fail(char const* message, T&& a, U&& b, util::SourceLocation loc);

constexpr void do_assert(bool b, char const* message, util::SourceLocation loc) {
    if (!b) {
        if consteval {
            di::util::compile_time_fail<>();
        } else {
            do_assert_fail(message, loc);
        }
    }
}

template<typename F, typename T, typename U>
constexpr void do_binary_assert(F op, char const* message, T&& a, U&& b, util::SourceLocation loc) {
    if (!op(a, b)) {
        if consteval {
            di::util::compile_time_fail<>();
        } else {
            do_binary_assert_fail(message, util::forward<T>(a), util::forward<U>(b), loc);
        }
    }
}
}

#define DI_ASSERT(...) \
    di::assert::detail::do_assert(bool(__VA_ARGS__), "" #__VA_ARGS__, di::util::SourceLocation::current())
#define DI_ASSERT_EQ(a, b)                                                               \
    di::assert::detail::do_binary_assert(di::function::equal, "" #a " == " #b, (a), (b), \
                                         di::util::SourceLocation::current())
#define DI_ASSERT_NOT_EQ(a, b)                                                               \
    di::assert::detail::do_binary_assert(di::function::not_equal, "" #a " != " #b, (a), (b), \
                                         di::util::SourceLocation::current())
#define DI_ASSERT_LT(a, b)                                                             \
    di::assert::detail::do_binary_assert(di::function::less, "" #a " < " #b, (a), (b), \
                                         di::util::SourceLocation::current())
#define DI_ASSERT_LT_EQ(a, b)                                                                    \
    di::assert::detail::do_binary_assert(di::function::equal_or_less, "" #a " <= " #b, (a), (b), \
                                         di::util::SourceLocation::current())
#define DI_ASSERT_GT(a, b)                                                                \
    di::assert::detail::do_binary_assert(di::function::greater, "" #a " > " #b, (a), (b), \
                                         di::util::SourceLocation::current())
#define DI_ASSERT_GT_EQ(a, b)                                                                       \
    di::assert::detail::do_binary_assert(di::function::equal_or_greater, "" #a " >= " #b, (a), (b), \
                                         di::util::SourceLocation::current())

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_ASSERT)
#define ASSERT        DI_ASSERT
#define ASSERT_EQ     DI_ASSERT_EQ
#define ASSERT_NOT_EQ DI_ASSERT_NOT_EQ
#define ASSERT_LT     DI_ASSERT_LT
#define ASSERT_LT_EQ  DI_ASSERT_LT_EQ
#define ASSERT_GT     DI_ASSERT_GT
#define ASSERT_GT_EQ  DI_ASSERT_GT_EQ
#endif
