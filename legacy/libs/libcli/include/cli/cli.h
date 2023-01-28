#pragma once

#include <cli/argument.h>
#include <cli/error.h>
#include <cli/flag.h>
#include <cli/parser.h>
#include <liim/vector.h>

// Requirements for command line argument parser:
// Handle boolean flags: -a, -v, --help, etc.
// Handle flags with arguments: -f file, -ffile, --file=file, --file file
// Multiple short arguments in one arg: -xzf
// Generate usage message, version information, man pages/markdown reference, shell completions?, etc.
// Position arguments: mv <source> <target>
// Variable argument lists, (in any position): mv <source...> <target>
// Parsing string types into c++ structures: use generic try_parse<> API
// Exclusivitiy of arguments: (POSIX cut -b,-c,-f)
// Generate cli information at compile time, use constexpr
// Make some arguments required
// Give arguments default values
// Extra features: handle alternate syntaxes (+e vs. -e), (-longopt), (posix find style arguments), (more weird posix utilities)
//                 sub-commands (git add, git remote, ...), powerful enough to implement libc getopt/getopt_long

#define CLI_MAIN(main_function, argument_parser)                         \
    int main(int argc, char** argv) {                                    \
        auto args = Vector<StringView> {};                               \
        for (int i = 0; i < argc; i++) {                                 \
            args.add(StringView { argv[i] });                            \
        }                                                                \
                                                                         \
        auto parse_result = argument_parser.parse(args.span());          \
        if (!parse_result) {                                             \
            error_log("{}: {}", args[0], parse_result.error());          \
            return 2;                                                    \
        }                                                                \
                                                                         \
        auto program_result = main_function(move(parse_result.value())); \
        if (!program_result) {                                           \
            error_log("{}: {}", args[0], program_result.error());        \
            return 1;                                                    \
        }                                                                \
        return 0;                                                        \
    }

#define CLI_MAIN_LEGACY(main_function, argument_parser)         \
    int main(int argc, char** argv) {                           \
        auto args = Vector<StringView> {};                      \
        for (int i = 0; i < argc; i++) {                        \
            args.add(StringView { argv[i] });                   \
        }                                                       \
                                                                \
        auto parse_result = argument_parser.parse(args.span()); \
        if (!parse_result) {                                    \
            error_log("{}: {}", args[0], parse_result.error()); \
            return 2;                                           \
        }                                                       \
                                                                \
        return main_function(move(parse_result.value()));       \
    }
