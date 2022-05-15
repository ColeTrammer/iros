#pragma once

#include <cli/error.h>
#include <liim/result.h>
#include <liim/string_view.h>
#include <liim/try.h>
#include <liim/vector.h>

namespace Cli {
class Argument {
private:
    using ParserCallback = Result<Monostate, Error> (*)(StringView, void*);

    template<auto member>
    class ArgumentBuilder {};

    template<typename StructType, typename ValueType, ValueType StructType::*member>
    class ArgumentBuilder<member> {
    public:
        static constexpr ArgumentBuilder single(StringView name) {
            return ArgumentBuilder(
                [](auto input, void* output_ptr) -> Result<Monostate, Error> {
                    auto& output = *static_cast<StructType*>(output_ptr);
                    auto value = TRY(Ext::parse<ValueType>(input));
                    output.*member = move(value);
                    return Ok(Monostate {});
                },
                name, false);
        }

        constexpr ArgumentBuilder(ParserCallback parser, StringView name, bool is_list)
            : m_parser(parser), m_name(name), m_is_list(is_list) {}

        constexpr ArgumentBuilder& description(StringView description) {
            m_description = description;
            return *this;
        }

        constexpr Argument argument() const { return Argument(m_parser, m_name, move(m_description), m_is_list); }

        constexpr operator Argument() const { return argument(); }

    private:
        ParserCallback m_parser;
        StringView m_name;
        Option<StringView> m_description;
        bool m_is_list { false };
    };

public:
    template<auto member>
    static constexpr ArgumentBuilder<member> single(StringView positional_name) {
        return ArgumentBuilder<member>::single(positional_name);
    }

    constexpr Argument() = default;

    constexpr StringView name() const { return m_name; }
    constexpr Option<StringView> description() const { return m_description; }
    constexpr bool is_list() const { return m_is_list; }

    constexpr Result<Monostate, Error> validate(StringView value, void* output) const { return m_parser(value, output); }

private:
    constexpr Argument(ParserCallback parser, StringView positional_name, Option<StringView> description, bool is_list)
        : m_parser(parser), m_name(positional_name), m_description(move(description)), m_is_list { is_list } {}

    ParserCallback m_parser;
    StringView m_name;
    Option<StringView> m_description;
    bool m_is_list { false };
};
}
