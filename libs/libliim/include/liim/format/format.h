#pragma once

#include <liim/container.h>
#include <liim/format/format_args_storage.h>
#include <liim/format/format_context.h>
#include <liim/format/format_parse_context.h>
#include <liim/variant.h>

namespace LIIM::Format {
template<typename T>
using FormatterType = Formatter<typename RemoveCVRef<T>::type>;

template<typename T>
concept Formattable = requires(T value, FormatterType<T> formatter, FormatParseContext parse_context, FormatContext context) {
    formatter.parse(parse_context);
    formatter.format(value, context);
};

class FormatStringIterator : public ValueIteratorAdapter<FormatStringIterator> {
public:
    struct Literal {
        StringView contents;
    };
    struct PlaceHolder {
        StringView contents;
    };
    using ValueType = Variant<Literal, PlaceHolder>;

    constexpr FormatStringIterator(StringView format_string) : m_format_string(format_string) {}

    constexpr Option<ValueType> next() {
        if (m_format_string.empty()) {
            return {};
        }

        auto process_literal = [&](size_t literal_size) -> ValueType {
            auto literal = m_format_string.first(literal_size);
            if (auto right_brace = literal.index_of('}')) {
                assert(literal.substring(*right_brace).starts_with("}}"));
                auto result = Literal(literal.first(*right_brace + 1));
                m_format_string = m_format_string.substring(*right_brace + 2);
                return { result };
            }
            m_format_string = m_format_string.substring(literal_size);
            return { Literal(literal) };
        };

        auto left_brace = m_format_string.index_of('{');
        if (!left_brace) {
            return process_literal(m_format_string.size());
        }

        if (*left_brace > 0) {
            return process_literal(*left_brace);
        }

        m_format_string = m_format_string.substring(1);
        if (m_format_string.starts_with("{")) {
            return process_literal(1);
        }

        auto right_brace = m_format_string.index_of('}');
        if (!right_brace) {
            // Error: unmatched left brace in format string
            assert(false);
        }

        auto result = PlaceHolder(m_format_string.first(*right_brace));
        m_format_string = m_format_string.substring(*right_brace + 1);
        return { result };
    }

private:
    StringView m_format_string;
};

inline void vformat_to_context(FormatContext& context, StringView format_string, FormatArgs format_args) {
    auto parse_context = FormatParseContext {};

    for (auto piece : FormatStringIterator(format_string)) {
        if (auto literal = piece.get_if<FormatStringIterator::Literal>()) {
            context.put(literal->contents);
        } else if (auto placeholder = piece.get_if<FormatStringIterator::PlaceHolder>()) {
            auto format_specifier = placeholder->contents;
            auto arg_index = parse_context.parse_arg_index(format_specifier);
            assert(parse_context.parse_colon());
            assert(arg_index);

            auto arg = format_args.arg_at_index(*arg_index);
            assert(arg);

            arg->do_format(context, parse_context);
        }
    }
}

inline String vformat(StringView format_string, FormatArgs format_args) {
    auto result = ""s;
    auto context = FormatContext { [](StringView piece, void* result_ptr) {
                                      *static_cast<String*>(result_ptr) += String(piece);
                                  },
                                   &result };
    vformat_to_context(context, format_string, move(format_args));
    return result;
}

template<Formattable... Args>
class FormatStringImpl {
public:
    template<typename T>
    consteval FormatStringImpl(T data) : m_data(StringView(data)) {
        auto parse_context = FormatParseContext {};

        using CheckFunction = void (*)(FormatParseContext&);
        CheckFunction check_functions[] = { [](FormatParseContext& parse_context) {
            auto formatter = Formatter<typename RemoveCVRef<Args>::type>();
            formatter.parse(parse_context);
        }... };

        bool argument_was_used[sizeof...(Args)] = {};

        for (auto piece : FormatStringIterator(m_data)) {
            if (auto placeholder = piece.template get_if<FormatStringIterator::PlaceHolder>()) {
                auto format_specifier = placeholder->contents;
                auto arg_index = parse_context.parse_arg_index(format_specifier);
                assert(arg_index);
                assert(*arg_index < sizeof...(Args));

                assert(parse_context.parse_colon());
                check_functions[*arg_index](parse_context);
                argument_was_used[*arg_index] = true;
            }
        }

        for (auto was_used : argument_was_used) {
            assert(was_used);
        }
    }

    constexpr StringView data() const { return m_data; }

private:
    StringView m_data;
};

template<Formattable... Args>
using FormatString = FormatStringImpl<typename TypeIdentity<Args>::type...>;

template<Formattable... Args>
void format_to_context(FormatContext& context, FormatString<Args...> format_string, Args&&... args) {
    return vformat_to_context(context, format_string.data(), make_format_args(forward<Args>(args)...));
}

template<Formattable... Args>
String format(FormatString<Args...> format_string, Args&&... args) {
    return vformat(format_string.data(), make_format_args(forward<Args>(args)...));
}
}
