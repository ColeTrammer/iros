#pragma once

#include <liim/format/base_formatter.h>
#include <liim/format/format_args.h>
#include <liim/format/format_context.h>
#include <liim/maybe.h>
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
struct Formatter<Maybe<T>> : public BaseFormatter {
    void format(const Maybe<T>& value, FormatContext& context) {
        if (!value) {
            return format_string_view("None", context);
        }
        return format_string_view(Format::format("{}", *value).view(), context);
    }
};
}
