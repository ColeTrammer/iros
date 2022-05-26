#pragma once

#include <liim/format/base_formatter.h>
#include <liim/format/format.h>
#include <liim/format/format_args.h>
#include <liim/format/format_context.h>
#include <liim/option.h>
#include <liim/string.h>
#include <liim/string_view.h>

namespace LIIM::Format {
template<>
struct Formatter<StringView> : public BaseFormatter {
    void format(const StringView& value, FormatContext& context) { format_string_view(value, context); }
};

template<>
struct Formatter<String> : public BaseFormatter {
    void format(const String& value, FormatContext& context) { format_string_view(value.view(), context); }
};

template<>
struct Formatter<const char*> : public Formatter<StringView> {};

template<>
struct Formatter<char*> : public Formatter<StringView> {};

template<size_t N>
struct Formatter<char[N]> : public Formatter<StringView> {};

template<typename T>
struct Formatter<Option<T>> : public BaseFormatter {
    void format(const Option<T>& value, FormatContext& context) {
        if (!value) {
            return format_string_view("None", context);
        }
        return format_string_view(Format::format("Some({})", *value).view(), context);
    }
};

template<>
struct Formatter<char> : public Formatter<StringView> {
    void format(char value, FormatContext& context) { return format_string_view({ &value, 1 }, context); }
};

template<SignedIntegral T>
struct Formatter<T> : public BaseFormatter {
    void format(const T& value, FormatContext& context) { return format_signed_integer(value, context); }
};

template<UnsignedIntegral T>
struct Formatter<T> : public BaseFormatter {
    void format(const T& value, FormatContext& context) { return format_unsigned_integer(value, context); }
};

template<typename T>
requires(!SameAs<T*, char> && !SameAs<T*, const char>) struct Formatter<T*> : public BaseFormatter {
    void format(T* value, FormatContext& context) {
        if (!value) {
            return format_string_view("Null"sv, context);
        }
        if constexpr (sizeof(uintptr_t) == 8) {
            return format_string_view(Format::format("{:#.16X}", (uintptr_t) value).view(), context);
        } else {
            return format_string_view(Format::format("{:#.8X}", (uintptr_t) value).view(), context);
        }
    }
};
}
