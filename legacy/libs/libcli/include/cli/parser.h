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

    constexpr ParserBuilder(FixedArray<Flag, flag_count> flags, FixedArray<Argument, argument_count> arguments, bool help_flag,
                            bool version_flag)
        : m_flags(move(flags)), m_arguments(move(arguments)), m_help_flag(help_flag), m_version_flag(version_flag) {}

    constexpr auto flag(Flag flag) {
        FixedArray<Flag, flag_count + 1> new_flags;
        for (size_t i = 0; i < flag_count; i++) {
            new_flags[i] = move(m_flags[i]);
        }
        new_flags[flag_count] = move(flag);
        return ParserBuilder<T, flag_count + 1, argument_count>(move(new_flags), move(m_arguments), m_help_flag, m_version_flag);
    }

    constexpr auto argument(Argument argument) {
        FixedArray<Argument, argument_count + 1> new_arguments;
        for (size_t i = 0; i < argument_count; i++) {
            new_arguments[i] = move(m_arguments[i]);
        }
        new_arguments[argument_count] = move(argument);
        return ParserBuilder<T, flag_count, argument_count + 1>(move(m_flags), move(new_arguments), m_help_flag, m_version_flag);
    }

    Result<T, Error> parse(Span<StringView> input) const {
        auto output = T {};
        ParserImpl parser_impl { m_flags.span(), m_arguments.span(), m_help_flag, m_version_flag };
        return parser_impl.parse(input, &output).transform([&output]() -> T {
            return move(output);
        });
    }

private:
    FixedArray<Flag, flag_count> m_flags;
    FixedArray<Argument, argument_count> m_arguments;
    bool m_help_flag { false };
    bool m_version_flag { false };
};
}

namespace Cli::Parser {
template<typename T>
static constexpr auto of() {
    return Cli::Detail::ParserBuilder<T, 0, 0>({}, {}, true, true);
}
}
