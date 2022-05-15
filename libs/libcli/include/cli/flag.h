#pragma once

#include <cli/error.h>
#include <liim/function.h>
#include <liim/option.h>
#include <liim/result.h>
#include <liim/string_view.h>

namespace Cli {
class Flag {
public:
    using ParserCallback = Result<Monostate, Error> (*)(Option<StringView>, void*);

    static constexpr Flag boolean(bool& value_out, Option<char> short_name, Option<StringView> long_name, Option<StringView> description) {
        return Flag(
            [](auto, void* value_out) -> Result<Monostate, Error> {
                *static_cast<bool*>(value_out) = true;
                return Ok(Monostate {});
            },
            &value_out, move(short_name), move(long_name), move(description));
    }

    template<typename T>
    static constexpr Flag value(Option<T>& value_out, Option<char> short_name, Option<StringView> long_name,
                                Option<StringView> description) {
        return Flag(
            [](auto, void*) -> Result<Monostate, Error> {
                return Ok(Monostate {});
            },
            &value_out, move(short_name), move(long_name), move(description));
    }

    constexpr Flag() = default;

    constexpr Option<char> short_name() const { return m_short_name; }
    constexpr Option<StringView> long_name() const { return m_long_name; }
    constexpr Option<StringView> description() const { return m_description; }

    constexpr bool requires_value() const { return false; }

    constexpr Result<Monostate, Error> validate(Option<StringView> value) const { return m_parser(move(value), m_parser_closure); }

private:
    constexpr Flag(ParserCallback parser, void* parser_closure, Option<char> short_name, Option<StringView> long_name,
                   Option<StringView> description)
        : m_parser(move(parser))
        , m_parser_closure(parser_closure)
        , m_short_name(move(short_name))
        , m_long_name(move(long_name))
        , m_description(move(description)) {}

    ParserCallback m_parser;
    void* m_parser_closure { nullptr };
    Option<char> m_short_name;
    Option<StringView> m_long_name;
    Option<StringView> m_description;
};
}
