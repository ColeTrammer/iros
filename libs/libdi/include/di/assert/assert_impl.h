#pragma once

#include <stdlib.h>
#include <unistd.h>

#include <di/container/string/fixed_string.h>
#include <di/format/concepts/formattable.h>
#include <di/format/to_string.h>
#include <di/util/compile_time_fail.h>

namespace di::assert::detail {
template<auto source_text>
constexpr void do_assert(bool b) {
    if (!b) {
        if consteval {
            di::util::compile_time_fail<source_text>();
        } else {
            auto new_line = '\n';
            [[maybe_unused]] ssize_t r;
            char text[] = "\033[31;1mASSERT\033[0m: ";
            r = ::write(2, text, sizeof(text) - 1);
            r = ::write(2, source_text.data(), source_text.size());
            r = ::write(2, &new_line, 1);
            ::abort();
        }
    }
}

template<auto source_text, typename F, typename T, typename U>
constexpr void do_binary_assert(F op, T&& a, U&& b) {
    if (!op(a, b)) {
        if consteval {
            di::util::compile_time_fail<source_text>();
        } else {
            auto new_line = '\n';
            [[maybe_unused]] ssize_t r;
            char text[] = "\033[31;1mASSERT\033[0m: ";
            r = ::write(2, text, sizeof(text) - 1);
            r = ::write(2, source_text.data(), source_text.size());
            r = ::write(2, &new_line, 1);

            if constexpr (concepts::Formattable<T> && concepts::Formattable<U>) {
                auto s = di::format::to_string(a);
                auto t = di::format::to_string(b);
                char lhs_text[] = "\033[1mLHS\033[0m: ";
                char rhs_text[] = "\n\033[1mRHS\033[0m: ";
                r = ::write(2, lhs_text, sizeof(lhs_text) - 1);
                r = ::write(2, s.data(), s.size());
                r = ::write(2, rhs_text, sizeof(rhs_text) - 1);
                r = ::write(2, t.data(), t.size());
                r = ::write(2, &new_line, 1);
            }

            ::abort();
        }
    }
}
}