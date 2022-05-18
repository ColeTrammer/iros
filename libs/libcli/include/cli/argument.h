#pragma once

#include <cli/error.h>
#include <cli/meta.h>
#include <liim/result.h>
#include <liim/string_view.h>
#include <liim/try.h>
#include <liim/vector.h>

namespace Cli {
class Argument {
private:
    using SingleParserCallback = Result<Monostate, Error> (*)(StringView, void*);
    using ListParserCallback = Result<Monostate, Error> (*)(Span<StringView>, void*, StringView name);

    using ParserCallback = union {
        SingleParserCallback single;
        ListParserCallback list;
    };

    template<auto member>
    class ArgumentBuilder {};

    template<typename StructType, typename ValueType, ValueType StructType::*member>
    class ArgumentBuilder<member> {
    private:
        static constexpr bool is_list = IsVector<ValueType>::value;
        using VectorParserType = IsVector<ValueType>::Type;
        static constexpr bool is_option = IsOption<VectorParserType>::value;
        using ParserType = IsOption<VectorParserType>::Type;

        constexpr ArgumentBuilder(SingleParserCallback parser, StringView name) : m_parser({ .single = parser }), m_name(name) {}
        constexpr ArgumentBuilder(ListParserCallback parser, StringView name) : m_parser({ .list = parser }), m_name(name) {}

    public:
        static constexpr ArgumentBuilder single(StringView name) requires(!is_list) {
            return ArgumentBuilder(
                [](StringView input, void* output_ptr) -> Result<Monostate, Error> {
                    auto& output = *static_cast<StructType*>(output_ptr);
                    auto value = TRY(Ext::parse<ParserType>(input));
                    output.*member = move(value);
                    return Ok(Monostate {});
                },
                name);
        }

        static constexpr ArgumentBuilder optional(StringView name) requires(is_option) {
            return ArgumentBuilder(
                [](StringView input, void* output_ptr) -> Result<Monostate, Error> {
                    auto& output = *static_cast<StructType*>(output_ptr);
                    auto value = TRY(Ext::parse<ParserType>(input));
                    output.*member = move(value);
                    return Ok(Monostate {});
                },
                name);
        }

        static constexpr ArgumentBuilder list(StringView name) requires(is_list) {
            return ArgumentBuilder(
                [](Span<StringView> input_list, void* output_ptr, StringView name) -> Result<Monostate, Error> {
                    auto& output = *static_cast<StructType*>(output_ptr);
                    if (input_list.empty()) {
                        return Err(EmptyPositionalArgumentList(name));
                    }
                    for (auto& input : input_list) {
                        auto value = TRY(Ext::parse<ParserType>(input));
                        (output.*member).add(move(value));
                    }
                    return Ok(Monostate {});
                },
                name);
        }

        constexpr ArgumentBuilder& description(StringView description) {
            m_description = description;
            return *this;
        }

        constexpr Argument argument() const { return Argument(m_parser, m_name, move(m_description), is_list, is_option); }

        constexpr operator Argument() const { return argument(); }

    private:
        ParserCallback m_parser;
        StringView m_name;
        Option<StringView> m_description;
    };

public:
    template<auto member>
    static constexpr ArgumentBuilder<member> single(StringView positional_name) {
        return ArgumentBuilder<member>::single(positional_name);
    }

    template<auto member>
    static constexpr ArgumentBuilder<member> optional(StringView positional_name) {
        return ArgumentBuilder<member>::optional(positional_name);
    }

    template<auto member>
    static constexpr ArgumentBuilder<member> list(StringView positional_name) {
        return ArgumentBuilder<member>::list(positional_name);
    }

    constexpr Argument() = default;

    constexpr StringView name() const { return m_name; }
    constexpr Option<StringView> description() const { return m_description; }
    constexpr bool is_list() const { return m_is_list; }
    constexpr bool is_optional() const { return m_is_optional; }

    Result<Monostate, Error> validate(StringView value, void* output) const { return m_parser.single(value, output); }
    Result<Monostate, Error> validate(Span<StringView> value, void* output) const { return m_parser.list(value, output, name()); }

private:
    constexpr Argument(ParserCallback parser, StringView positional_name, Option<StringView> description, bool is_list, bool is_optional)
        : m_parser(parser), m_name(positional_name), m_description(move(description)), m_is_list(is_list), m_is_optional(is_optional) {}

    ParserCallback m_parser;
    StringView m_name;
    Option<StringView> m_description;
    bool m_is_list { false };
    bool m_is_optional { false };
};
}
