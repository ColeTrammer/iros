#pragma once

#include "di/container/iterator/distance.h"
#include "di/format/bounded_format_context.h"
#include "di/format/builtin_formatter/prelude.h"
#include "di/format/concepts/formattable.h"
#include "di/format/to_string.h"
#include "di/function/equal.h"
#include "di/function/equal_or_greater.h"
#include "di/function/equal_or_less.h"
#include "di/function/greater.h"
#include "di/function/less.h"
#include "di/function/not_equal.h"
#include "di/math/functions.h"
#include "di/math/to_unsigned.h"
#include "di/meta/compare.h"
#include "di/util/compile_time_fail.h"
#include "di/util/source_location.h"

namespace di::assert::detail {
template<typename T, typename U>
void binary_assert_fail(char const* expression, T&& a, U&& b, util::SourceLocation loc) {
    // Allocate a 512 byte buffer on the stack to stringify a and b. We'd don't want to allocate here because this
    // assertion could indicate heap corruption, or even be triggered before the heap is initialized.
    using Enc = container::string::Utf8Encoding;
    using TargetContext = format::BoundedFormatContext<Enc, meta::Constexpr<512ZU>>;

    auto lhs_context = TargetContext {};
    auto const* lhs_data_pointer = static_cast<char const*>(nullptr);
    if constexpr (concepts::Formattable<T>) {
        (void) di::format::vpresent_encoded_context<Enc>(
            di::container::string::StringViewImpl<Enc>(encoding::assume_valid, u8"{}", 2),
            di::format::make_format_args<TargetContext>(a), lhs_context, true);
        lhs_data_pointer = reinterpret_cast<char const*>(lhs_context.output().span().data());
    }
    auto rhs_context = TargetContext {};
    auto const* rhs_data_pointer = static_cast<char const*>(nullptr);
    if constexpr (concepts::Formattable<U>) {
        (void) di::format::vpresent_encoded_context<Enc>(
            di::container::string::StringViewImpl<Enc>(encoding::assume_valid, u8"{}", 2),
            di::format::make_format_args<TargetContext>(b), rhs_context, true);
        rhs_data_pointer = reinterpret_cast<char const*>(rhs_context.output().span().data());
    }
    assert_fail(expression, lhs_data_pointer, rhs_data_pointer, loc);
}

template<typename F, typename T, typename U>
constexpr void binary_assert(F op, char const* expression, T&& a, U&& b, util::SourceLocation loc) {
    if (!op(a, b)) {
        if consteval {
            ::di::util::compile_time_fail<>();
        } else {
            binary_assert_fail(expression, util::forward<T>(a), util::forward<U>(b), loc);
        }
    }
}
}

#define DI_ASSERT_EQ(a, b)                                                                \
    ::di::assert::detail::binary_assert(::di::function::equal, "" #a " == " #b, (a), (b), \
                                        ::di::util::SourceLocation::current())

#define DI_ASSERT_APPROX_EQ(a, b)                                                             \
    ::di::assert::detail::binary_assert(::di::approximately_equal, "" #a " ~= " #b, (a), (b), \
                                        ::di::util::SourceLocation::current())

#define DI_ASSERT_NOT_EQ(a, b)                                                                \
    ::di::assert::detail::binary_assert(::di::function::not_equal, "" #a " != " #b, (a), (b), \
                                        ::di::util::SourceLocation::current())
#define DI_ASSERT_LT(a, b)                                                              \
    ::di::assert::detail::binary_assert(::di::function::less, "" #a " < " #b, (a), (b), \
                                        ::di::util::SourceLocation::current())
#define DI_ASSERT_LT_EQ(a, b)                                                                     \
    ::di::assert::detail::binary_assert(::di::function::equal_or_less, "" #a " <= " #b, (a), (b), \
                                        ::di::util::SourceLocation::current())
#define DI_ASSERT_GT(a, b)                                                                 \
    ::di::assert::detail::binary_assert(::di::function::greater, "" #a " > " #b, (a), (b), \
                                        ::di::util::SourceLocation::current())
#define DI_ASSERT_GT_EQ(a, b)                                                                        \
    ::di::assert::detail::binary_assert(::di::function::equal_or_greater, "" #a " >= " #b, (a), (b), \
                                        ::di::util::SourceLocation::current())

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_ASSERT)
#define ASSERT_EQ        DI_ASSERT_EQ
#define ASSERT_APPROX_EQ DI_ASSERT_APPROX_EQ
#define ASSERT_NOT_EQ    DI_ASSERT_NOT_EQ
#define ASSERT_LT        DI_ASSERT_LT
#define ASSERT_LT_EQ     DI_ASSERT_LT_EQ
#define ASSERT_GT        DI_ASSERT_GT
#define ASSERT_GT_EQ     DI_ASSERT_GT_EQ
#endif
