#pragma once

#include <cli/error.h>
#include <liim/function.h>
#include <liim/option.h>
#include <liim/result.h>
#include <liim/string_view.h>

namespace Cli {
class Flag {
private:
    using ParserCallback = Result<Monostate, Error> (*)(Option<StringView>, void*);

    class FlagBuilder {
    public:
        constexpr FlagBuilder(ParserCallback parser, void* parser_closure, bool requires_value)
            : m_parser(parser), m_parser_closure(parser_closure), m_requires_value(requires_value) {}

        constexpr FlagBuilder& short_name(char name) {
            m_short_name = name;
            return *this;
        }

        constexpr FlagBuilder& long_name(StringView name) {
            m_long_name = name;
            return *this;
        }

        constexpr FlagBuilder& description(StringView description) {
            m_description = description;
            return *this;
        }

        constexpr Flag flag() const {
            return Flag(m_parser, m_parser_closure, move(m_short_name), move(m_long_name), move(m_description), m_requires_value);
        }

        constexpr operator Flag() const { return flag(); }

    private:
        ParserCallback m_parser;
        void* m_parser_closure { nullptr };
        Option<char> m_short_name;
        Option<StringView> m_long_name;
        Option<StringView> m_description;
        bool m_requires_value { false };
    };

public:
    static constexpr FlagBuilder boolean(bool& value_out) {
        return FlagBuilder(
            [](auto, void* value_out) -> Result<Monostate, Error> {
                *static_cast<bool*>(value_out) = true;
                return Ok(Monostate {});
            },
            &value_out, false);
    }

    template<typename T>
    static constexpr FlagBuilder value(Option<T>& value_out) {
        return FlagBuilder(
            [](auto, void*) -> Result<Monostate, Error> {
                return Ok(Monostate {});
            },
            &value_out, true);
    }

    constexpr Flag() = default;

    constexpr Option<char> short_name() const { return m_short_name; }
    constexpr Option<StringView> long_name() const { return m_long_name; }
    constexpr Option<StringView> description() const { return m_description; }

    constexpr bool requires_value() const { return false; }

    constexpr Result<Monostate, Error> validate(Option<StringView> value) const { return m_parser(move(value), m_parser_closure); }

private:
    constexpr Flag(ParserCallback parser, void* parser_closure, Option<char> short_name, Option<StringView> long_name,
                   Option<StringView> description, bool requires_value)
        : m_parser(parser)
        , m_parser_closure(parser_closure)
        , m_short_name(move(short_name))
        , m_long_name(move(long_name))
        , m_description(move(description))
        , m_requires_value(requires_value) {}

    ParserCallback m_parser;
    void* m_parser_closure { nullptr };
    Option<char> m_short_name;
    Option<StringView> m_long_name;
    Option<StringView> m_description;
    bool m_requires_value { false };
};
}
