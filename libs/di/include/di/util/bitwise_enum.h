#pragma once

#include "di/util/to_underlying.h"

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
