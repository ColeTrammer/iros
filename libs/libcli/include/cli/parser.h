#pragma once

#include <cli/argument.h>
#include <cli/error.h>
#include <cli/flag.h>
#include <cli/parser_impl.h>
#include <liim/fixed_array.h>
#include <liim/result.h>
#include <liim/vector.h>

namespace Cli {
template<size_t flag_count, size_t argument_count>
class Parser {
public:
    constexpr Parser(FixedArray<Flag, flag_count> flags, FixedArray<Argument, argument_count> arguments)
        : m_flags(move(flags)), m_arguments(move(arguments)) {}

    Result<Monostate, Error> parse(Span<StringView> input) const {
        Detail::ParserImpl parser_impl { m_flags.span(), m_arguments.span() };
        return parser_impl.parse(input);
    }

private:
    FixedArray<Flag, flag_count> m_flags;
    FixedArray<Argument, argument_count> m_arguments;
};
}
