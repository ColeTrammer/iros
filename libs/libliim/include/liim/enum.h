#pragma once

#include <assert.h>
#include <liim/format.h>

#define __LIIM_GENERATE_ENUM(x) x,
#define __LIIM_FORMAT_ENUM(x) \
    case x:                   \
        return Formatter<StringView>::format(StringView("" #x), context);

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
