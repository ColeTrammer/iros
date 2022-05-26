#pragma once

#include <liim/container.h>
#include <liim/format/format_args_storage.h>
#include <liim/format/format_context.h>
#include <liim/format/format_parse_context.h>
#include <liim/variant.h>

namespace LIIM::Format {
class FormatStringIterator : public ValueIteratorAdapter<FormatStringIterator> {
public:
    struct Literal {
        StringView contents;
    };
    struct PlaceHolder {
        StringView contents;
    };
    using ValueTypeInner = Variant<Literal, PlaceHolder>;
    using ValueType = Option<ValueTypeInner>;

    constexpr FormatStringIterator(StringView format_string) : m_format_string(format_string) {}

    constexpr Option<ValueType> next() {
        if (m_format_string.empty()) {
            return {};
        }

        auto left_brace = m_format_string.index_of('{');
        if (!left_brace) {
            auto result = ValueTypeInner(Literal(m_format_string));
            m_format_string = {};
            return { result };
        }

        if (*left_brace > 0) {
            auto result = ValueTypeInner(Literal(m_format_string.first(*left_brace)));
            m_format_string = m_format_string.substring(*left_brace);
            return { result };
        }

        m_format_string = m_format_string.substring(1);
        if (m_format_string.starts_with("{")) {
            auto result = ValueTypeInner(Literal("{{"sv));
            m_format_string = m_format_string.substring(1);
            return { result };
        }

        auto right_brace = m_format_string.index_of('}');
        if (!right_brace) {
            // Error: unmatched left brace in format string
            return { None {} };
        }

        auto result = ValueTypeInner(PlaceHolder(m_format_string.first(*right_brace)));
        m_format_string = m_format_string.substring(*right_brace + 1);
        return { result };
    }

private:
    StringView m_format_string;
};

inline String vformat(StringView format_string, FormatArgs format_args) {
    auto parse_context = FormatParseContext {};
    auto context = FormatContext {};

    for (auto piece : FormatStringIterator(format_string)) {
        assert(piece);
        if (auto literal = piece->get_if<FormatStringIterator::Literal>()) {
            context.put(literal->contents);
        } else if (auto placeholder = piece->get_if<FormatStringIterator::PlaceHolder>()) {
            auto format_specifier = placeholder->contents;
            auto arg_index = parse_context.parse_arg_index(format_specifier);
            assert(parse_context.parse_colon());
            assert(arg_index);

            auto arg = format_args.arg_at_index(*arg_index);
            assert(arg);

            arg->do_format(context, parse_context);
        }
    }

    return context.take_accumulator();
}

template<typename... Args>
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
            if (!piece) {
                assert(false);
            }

            if (auto placeholder = piece.value().template get_if<FormatStringIterator::PlaceHolder>()) {
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

template<typename... Args>
using FormatString = FormatStringImpl<typename TypeIdentity<Args>::type...>;

template<typename... Args>
String format(FormatString<Args...> format_string, Args&&... args) {
    return vformat(format_string.data(), make_format_args(forward<Args>(args)...));
}
}
