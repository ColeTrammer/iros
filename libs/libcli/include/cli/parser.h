#pragma once

#include <cli/argument.h>
#include <cli/error.h>
#include <cli/flag.h>
#include <cli/parser_impl.h>
#include <liim/fixed_array.h>
#include <liim/result.h>
#include <liim/vector.h>

namespace Cli::Detail {
template<typename T, size_t flag_count, size_t argument_count>
class ParserBuilder {
public:
    using OutputType = T;

    constexpr ParserBuilder(FixedArray<Flag, flag_count> flags, FixedArray<Argument, argument_count> arguments)
        : m_flags(move(flags)), m_arguments(move(arguments)) {}

    constexpr auto flag(Flag flag) {
        FixedArray<Flag, flag_count + 1> new_flags;
        for (size_t i = 0; i < flag_count; i++) {
            new_flags[i] = move(m_flags[i]);
        }
        new_flags[flag_count] = move(flag);
        return ParserBuilder<T, flag_count + 1, argument_count>(move(new_flags), move(m_arguments));
    }

    constexpr auto argument(Argument argument) {
        FixedArray<Argument, argument_count + 1> new_arguments;
        for (size_t i = 0; i < argument_count; i++) {
            new_arguments[i] = move(m_arguments[i]);
        }
        new_arguments[argument_count] = move(argument);
        return ParserBuilder<T, flag_count, argument_count + 1>(move(m_flags), move(new_arguments));
    }

    Result<T, Error> parse(Span<StringView> input) const {
        auto output = T {};
        ParserImpl parser_impl { m_flags.span(), m_arguments.span() };
        return parser_impl.parse(input, &output).map([&output](auto) -> T {
            return move(output);
        });
    }

private:
    FixedArray<Flag, flag_count> m_flags;
    FixedArray<Argument, argument_count> m_arguments;
};
}

namespace Cli::Parser {
template<typename T>
static constexpr auto of() {
    return Cli::Detail::ParserBuilder<T, 0, 0>({}, {});
}
}
