#pragma once

#include <assert.h>
#include <liim/format.h>
#include <liim/utilities.h>

#define __LIIM_GENERATE_ENUM(x) x,
#define __LIIM_FORMAT_ENUM(x) \
    case x:                   \
        return format_string_view("" #x, context);

#define LIIM_ENUM(Namespace, Name, macro)                                         \
    namespace Namespace {                                                         \
    enum class Name { macro(__LIIM_GENERATE_ENUM) };                              \
    }                                                                             \
    namespace LIIM::Format {                                                      \
    template<>                                                                    \
    struct Formatter<Namespace::Name> : public Formatter<StringView> {            \
        void format(Namespace::Name value, FormatContext& context) {              \
            using enum Namespace::Name;                                           \
            switch (value) { macro(__LIIM_FORMAT_ENUM) default : assert(false); } \
        }                                                                         \
    };                                                                            \
    }

#define LIIM_DEFINE_BITWISE_OPERATIONS(Enum)                           \
    static_assert(Enumeration<Enum>);                                  \
    constexpr Enum operator~(Enum a) {                                 \
        return static_cast<Enum>(~to_underlying(a));                   \
    }                                                                  \
    constexpr Enum operator|(Enum a, Enum b) {                         \
        return static_cast<Enum>(to_underlying(a) | to_underlying(b)); \
    }                                                                  \
    constexpr Enum operator&(Enum a, Enum b) {                         \
        return static_cast<Enum>(to_underlying(a) & to_underlying(b)); \
    }                                                                  \
    constexpr Enum operator^(Enum a, Enum b) {                         \
        return static_cast<Enum>(to_underlying(a) ^ to_underlying(b)); \
    }                                                                  \
    constexpr Enum& operator|=(Enum& a, Enum b) {                      \
        return a = a | b;                                              \
    }                                                                  \
    constexpr Enum& operator&=(Enum& a, Enum b) {                      \
        return a = a & b;                                              \
    }                                                                  \
    constexpr Enum& operator^=(Enum& a, Enum b) {                      \
        return a = a ^ b;                                              \
    }
