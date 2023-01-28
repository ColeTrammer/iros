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
template<auto source_text>
constexpr void do_assert(bool b, util::SourceLocation loc);

template<auto source_text, typename F, typename T, typename U>
constexpr void do_binary_assert(F op, T&& a, U&& b, util::SourceLocation loc);
}

#define DI_ASSERT(...)                                                                               \
    di::assert::detail::do_assert<di::container::FixedString { "" #__VA_ARGS__ }>(bool(__VA_ARGS__), \
                                                                                  di::util::SourceLocation::current())
#define DI_ASSERT_EQ(a, b)                                                                \
    di::assert::detail::do_binary_assert<di::container::FixedString { "" #a " == " #b }>( \
        di::function::equal, (a), (b), di::util::SourceLocation::current())
#define DI_ASSERT_NOT_EQ(a, b)                                                            \
    di::assert::detail::do_binary_assert<di::container::FixedString { "" #a " != " #b }>( \
        di::function::not_equal, (a), (b), di::util::SourceLocation::current())
#define DI_ASSERT_LT(a, b)                                                               \
    di::assert::detail::do_binary_assert<di::container::FixedString { "" #a " < " #b }>( \
        di::function::less, (a), (b), di::util::SourceLocation::current())
#define DI_ASSERT_LT_EQ(a, b)                                                             \
    di::assert::detail::do_binary_assert<di::container::FixedString { "" #a " <= " #b }>( \
        di::function::equal_or_less, (a), (b), di::util::SourceLocation::current())
#define DI_ASSERT_GT(a, b)                                                               \
    di::assert::detail::do_binary_assert<di::container::FixedString { "" #a " > " #b }>( \
        di::function::greater, (a), (b), di::util::SourceLocation::current())
#define DI_ASSERT_GT_EQ(a, b)                                                             \
    di::assert::detail::do_binary_assert<di::container::FixedString { "" #a " >= " #b }>( \
        di::function::equal_or_greater, (a), (b), di::util::SourceLocation::current())
