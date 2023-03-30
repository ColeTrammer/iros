#pragma once

#include <di/util/to_underlying.h>

#define DI_DEFINE_ENUM_BITWISE_OPERATIONS(Type)                                                \
    static_assert(::di::concepts::Enum<Type>);                                                 \
    constexpr Type operator~(Type a) {                                                         \
        return static_cast<Type>(~::di::util::to_underlying(a));                               \
    }                                                                                          \
    constexpr Type operator|(Type a, Type b) {                                                 \
        return static_cast<Type>(::di::util::to_underlying(a) | ::di::util::to_underlying(b)); \
    }                                                                                          \
    constexpr Type operator&(Type a, Type b) {                                                 \
        return static_cast<Type>(::di::util::to_underlying(a) & ::di::util::to_underlying(b)); \
    }                                                                                          \
    constexpr Type operator^(Type a, Type b) {                                                 \
        return static_cast<Type>(::di::util::to_underlying(a) ^ ::di::util::to_underlying(b)); \
    }                                                                                          \
    constexpr Type& operator|=(Type& a, Type b) {                                              \
        return a = a | b;                                                                      \
    }                                                                                          \
    constexpr Type& operator&=(Type& a, Type b) {                                              \
        return a = a & b;                                                                      \
    }                                                                                          \
    constexpr Type& operator^=(Type& a, Type b) {                                              \
        return a = a ^ b;                                                                      \
    }                                                                                          \
    constexpr bool operator!(Type a) {                                                         \
        return !::di::util::to_underlying(a);                                                  \
    }
