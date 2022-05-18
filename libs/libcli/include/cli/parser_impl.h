#pragma once

#include <cli/error.h>
#include <liim/forward.h>

namespace Cli::Detail {
class ParserImpl {
public:
    ParserImpl(Span<const Flag> flags, Span<const Argument> arguments, bool help_flag, bool version_flag);
    ~ParserImpl();

    Result<Monostate, Error> parse(Span<StringView> input, void* output) const;

private:
    void print_usage();
    void print_help();
    void print_version();

    Result<const Flag&, UnexpectedLongFlag> lookup_long_flag(StringView name) const;
    Result<const Flag&, UnexpectedShortFlag> lookup_short_flag(char name) const;

    Span<const Flag> m_flags;
    Span<const Argument> m_arguments;
    bool m_help_flag { false };
    bool m_version_flag { false };
};
}
