#pragma once

#ifndef DI_CUSTOM_ASSERT_HANDLER
#include <stdlib.h>
#include <unistd.h>
#endif

#include <di/container/string/fixed_string.h>
#include <di/format/concepts/formattable.h>
#include <di/format/to_string.h>
#include <di/util/compile_time_fail.h>

namespace di::assert::detail {
void assert_write(char const*, size_t);
void assert_terminate();

#ifndef DI_CUSTOM_ASSERT_HANDLER
inline void assert_write(char const* data, size_t size) {
    ssize_t r = ::write(2, data, size);
    (void) r;
}

inline void assert_terminate() {
    ::abort();
}
#endif

template<auto source_text>
constexpr void do_assert(bool b) {
    if (!b) {
        if consteval {
            di::util::compile_time_fail<source_text>();
        } else {
            auto new_line = '\n';
            char text[] = "\033[31;1mASSERT\033[0m: ";
            assert_write(text, sizeof(text) - 1);
            assert_write(source_text.data(), source_text.size());
            assert_write(&new_line, 1);
            assert_terminate();
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
            char text[] = "\033[31;1mASSERT\033[0m: ";
            assert_write(text, sizeof(text) - 1);
            assert_write(source_text.data(), source_text.size());
            assert_write(&new_line, 1);

#ifndef DI_NO_ASSERT_ALLOCATION
            if constexpr (concepts::Formattable<T> && concepts::Formattable<U>) {
                auto s = di::format::to_string(a);
                auto t = di::format::to_string(b);
                char lhs_text[] = "\033[1mLHS\033[0m: ";
                char rhs_text[] = "\n\033[1mRHS\033[0m: ";
                assert_write(lhs_text, sizeof(lhs_text) - 1);
                assert_write(reinterpret_cast<char const*>(s.data()), s.size_bytes());
                assert_write(rhs_text, sizeof(rhs_text) - 1);
                assert_write(reinterpret_cast<char const*>(t.data()), t.size_bytes());
                assert_write(&new_line, 1);
            }
#endif

            assert_terminate();
        }
    }
}
}