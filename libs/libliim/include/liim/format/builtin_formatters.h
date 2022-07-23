#pragma once

#include <liim/container/array.h>
#include <liim/container/erased_string.h>
#include <liim/format/base_formatter.h>
#include <liim/format/format.h>
#include <liim/format/format_args.h>
#include <liim/format/format_context.h>
#include <liim/option.h>
#include <liim/result.h>
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

template<>
struct Formatter<ErasedString> : public BaseFormatter {
    void format(const ErasedString& value, FormatContext& context) { format_string_view({ value.data(), value.size() }, context); }
};

template<>
struct Formatter<None> : public BaseFormatter {
    void format(None, FormatContext& context) { return format_string_view("None", context); }
};

template<Formattable T, size_t N>
struct Formatter<LIIM::Container::Array<T, N>> {
    constexpr void parse(FormatParseContext& context) { m_formatter.parse(context); }

    void format(const LIIM::Container::Array<T, N>& array, FormatContext& context) {
        context.put("[ ");
        bool first = true;
        for (auto& item : array) {
            if (!first) {
                context.put(", ");
            }
            m_formatter.format(item, context);
            first = false;
        }
        context.put(" ]");
    }

    Formatter<T> m_formatter;
};

template<Formattable T>
struct Formatter<Option<T>> : public BaseFormatter {
    void format(const Option<T>& value, FormatContext& context) {
        if (!value) {
            return format_string_view("None", context);
        }
        return format_to_context(context, "Some({})", *value);
    }
};

template<Formattable T, Formattable E>
struct Formatter<Result<T, E>> : public BaseFormatter {
    void format(const Result<T, E>& value, FormatContext& context) {
        if (value.is_error()) {
            return format_to_context(context, "Err({})", value.error());
        }
        return format_to_context(context, "Ok({})", value.value());
    }
};

template<Formattable E>
struct Formatter<Err<E>> : public BaseFormatter {
    void format(const Err<E>& value, FormatContext& context) { return format_to_context(context, "Err({})", value.error()); }
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
            return format_to_context(context, "{:#.16X}", (uintptr_t) value);
        } else {
            return format_to_context(context, "{:#.8X}", (uintptr_t) value);
        }
    }
};

template<>
struct Formatter<std::nullptr_t> : public BaseFormatter {
    void format(std::nullptr_t, FormatContext& context) { return format_string_view("Null"sv, context); }
};

template<typename T>
concept HasToString = requires(const T& x) {
    { x.to_string() } -> SameAs<String>;
};

template<HasToString T>
struct Formatter<T> {
    constexpr void parse(FormatParseContext&) {}
    void format(const T& value, FormatContext& context) { return context.put(value.to_string().view()); }
};
}
