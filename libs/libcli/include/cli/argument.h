#pragma once

#include <cli/error.h>
#include <liim/result.h>
#include <liim/string_view.h>
#include <liim/vector.h>

namespace Cli {
class Argument {
public:
    using ParserCallback = Result<Monostate, Error> (*)(StringView, void*);

    template<typename T>
    static constexpr Argument with(T& value_out, StringView positional_name, Option<StringView> description) {
        return Argument(
            [](auto value, void* value_out_ptr) -> Result<Monostate, Error> {
                auto& value_out = *static_cast<T*>(value_out_ptr);
                value_out = value;
                return Ok(Monostate {});
            },
            &value_out, move(positional_name), move(description), false);
    }

    template<typename T>
    static constexpr Argument list(Vector<T>& value_out, StringView positional_name, Option<StringView> description) {
        return Argument(
            [](auto, auto) -> Result<Monostate, Error> {
                return Ok(Monostate {});
            },
            &value_out, move(positional_name), move(description), true);
    }

    constexpr Argument() = default;

    constexpr StringView name() const { return m_name; }
    constexpr Option<StringView> description() const { return m_description; }
    constexpr bool is_list() const { return m_is_list; }

    constexpr Result<Monostate, Error> validate(StringView value) const { return m_parser(value, m_parser_closure); }

private:
    constexpr Argument(ParserCallback parser, void* parser_closure, StringView positional_name, Option<StringView> description,
                       bool is_list)
        : m_parser(parser)
        , m_parser_closure(parser_closure)
        , m_name(positional_name)
        , m_description(move(description))
        , m_is_list { is_list } {}

    ParserCallback m_parser;
    void* m_parser_closure { nullptr };
    StringView m_name;
    Option<StringView> m_description;
    bool m_is_list { false };
};
}
