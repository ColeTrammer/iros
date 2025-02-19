#pragma once

#include "di/meta/core.h"
#include "di/meta/language.h"
#include "di/util/to_underlying.h"

namespace di::concepts {
template<typename T>
concept BitwiseEnum = concepts::Enum<T> && requires(T& lvalue, T a) {
    { ~a } -> di::SameAs<T>;
    { a | a } -> di::SameAs<T>;
    { a& a } -> di::SameAs<T>;
    { a ^ a } -> di::SameAs<T>;
    { lvalue |= a } -> di::SameAs<T&>;
    { lvalue &= a } -> di::SameAs<T&>;
    { lvalue ^= a } -> di::SameAs<T&>;
};
}

#define DI_DEFINE_ENUM_BITWISE_OPERATIONS(Type)                                                \
    static_assert(::di::concepts::Enum<Type>);                                                 \
    constexpr auto operator~(Type a)->Type {                                                   \
        return static_cast<Type>(~::di::util::to_underlying(a));                               \
    }                                                                                          \
    constexpr auto operator|(Type a, Type b)->Type {                                           \
        return static_cast<Type>(::di::util::to_underlying(a) | ::di::util::to_underlying(b)); \
    }                                                                                          \
    constexpr auto operator&(Type a, Type b)->Type {                                           \
        return static_cast<Type>(::di::util::to_underlying(a) & ::di::util::to_underlying(b)); \
    }                                                                                          \
    constexpr auto operator^(Type a, Type b)->Type {                                           \
        return static_cast<Type>(::di::util::to_underlying(a) ^ ::di::util::to_underlying(b)); \
    }                                                                                          \
    constexpr auto operator|=(Type& a, Type b)->Type& {                                        \
        return a = a | b;                                                                      \
    }                                                                                          \
    constexpr auto operator&=(Type& a, Type b)->Type& {                                        \
        return a = a & b;                                                                      \
    }                                                                                          \
    constexpr auto operator^=(Type& a, Type b)->Type& {                                        \
        return a = a ^ b;                                                                      \
    }                                                                                          \
    constexpr auto operator!(Type a)->bool {                                                   \
        return !::di::util::to_underlying(a);                                                  \
    }
