#include <ccpp/bits/getopt_implementation.h>
#include <di/prelude.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>

namespace ccpp {
static char* next_short_char = nullptr;

static di::Optional<int> handle_short_options(int argc, di::Span<char*> argv, di::TransparentStringView opts,
                                              bool strict_mode, bool print_errors) {
    // There are short options if next_short_char is set.
    if (!next_short_char) {
        return di::nullopt;
    }

    // Reset if there are no short options left.
    if (*next_short_char == '\0') {
        next_short_char = nullptr;
        optind++;
        return di::nullopt;
    }

    // Look up the short name in opts.
    auto short_name = *next_short_char++;
    auto [match, after_match] = opts.find(short_name);
    if (match == opts.end()) {
        if (print_errors) {
            (void) fprintf(stderr, "%s: unknown argument '-%c'\n", argv[0], short_name);
        }
        optopt = short_name;
        return '?';
    }

    // If a value is required, either use the rest of the current argument or the next argument.
    if (*after_match == ':') {
        auto* value = next_short_char;
        if (*value == '\0') {
            if (optind + 1 >= argc) {
                if (print_errors) {
                    (void) fprintf(stderr, "%s: argument '-%c' must be passed a value\n", argv[0], short_name);
                }
                optopt = short_name;
                optind++;
                next_short_char = nullptr;
                return strict_mode ? ':' : '?';
            }
            value = argv[++optind];
        }

        optarg = value;
        optind++;
        next_short_char = nullptr;
        return short_name;
    }

    // No value, just return.
    optarg = nullptr;
    return short_name;
}

static di::Optional<int> handle_long_options(int argc, di::Span<char*> argv, di::TransparentStringView arg,
                                             di::Span<option const> longopts, int* longindex, bool strict_mode,
                                             bool print_errors, bool long_only) {
    // A candidate long argument either starts with "--" or "-" if long_only is true.
    auto long_arg = [&] -> di::Optional<di::TransparentStringView> {
        if (arg.starts_with("--"_tsv)) {
            return arg | di::drop(2);
        }
        if (long_only) {
            return arg | di::drop(1);
        }
        return di::nullopt;
    }();

    if (!long_arg) {
        return di::nullopt;
    }

    // Find the long name, which strips the argument value if it exists.
    auto [long_name, long_value] = [&] -> di::Tuple<di::TransparentStringView, char const*> {
        auto [equal, after_equal] = long_arg->find('=');
        if (equal != long_arg->end()) {
            long_arg->replace_end(equal);
            return di::make_tuple(*long_arg, after_equal);
        }
        return di::make_tuple(*long_arg, nullptr);
    }();

    // Match the long option.
    auto const* match = di::find_if(longopts, [&](option const& opt) {
        return di::container::equal(long_name, di::ZCString(opt.name));
    });
    if (match == longopts.end()) {
        // If long only is true, and the current argument only started with '-', we have to try short options.
        if (long_only && !arg.starts_with("--"_tsv)) {
            return di::nullopt;
        }
        if (print_errors) {
            (void) fprintf(stderr, "%s: unknown argument '%s'\n", argv[0], arg.data());
        }
        optind++;
        return '?';
    }

    switch (match->has_arg) {
        case no_argument: {
            if (long_value) {
                if (print_errors) {
                    (void) fprintf(stderr, "%s: argument '%s' cannot accept a value\n", argv[0], arg.data());
                }
                optind++;
                return strict_mode ? ':' : '?';
            }
            optarg = nullptr;
            break;
        }
        case optional_argument:
            if (long_value) {
                optarg = const_cast<char*>(long_value);
            } else {
                optarg = nullptr;
            }
            break;
        case required_argument:
            if (!long_value) {
                if (optind + 1 >= argc) {
                    if (print_errors) {
                        (void) fprintf(stderr, "%s: argument '%s' must be passed a value\n", argv[0], arg.data());
                    }
                    optind++;
                    return strict_mode ? ':' : '?';
                }
                optarg = argv[++optind];
            } else {
                optarg = const_cast<char*>(long_value);
            }
            break;
        default:
            di::unreachable();
    }

    // Return the found argument.
    optind++;
    if (longindex) {
        *longindex = int(match - longopts.begin());
    }
    if (match->flag) {
        *match->flag = match->val;
        return 0;
    }
    return match->val;
}

// This function implements both POSIX getopt and the GNU extension getopt_long.
// https://pubs.opengroup.org/onlinepubs/009696799/functions/getopt.html
// https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Options.html
int getopt_implementation(int argc, char* const argv_in[], char const* optstring, const struct option* longopts_in,
                          int* longindex, bool long_only) {
    // NOTE: this const cast is required because the getopt interface is not const correct (at least not the GNU
    // extension).
    auto argv = di::Span { (char**) argv_in, (char**) argv_in + argc };
    auto longopts = di::Span { longopts_in, 0 };
    if (longopts_in) {
        auto const* longopts_end = longopts_in;
        while (longopts_end->name != nullptr) {
            longopts_end++;
        }
        longopts = di::Span { longopts_in, longopts_end };
    }
    auto opts = di::TransparentStringView(optstring, strlen(optstring));
    auto strict_mode = false;
    if (opts.starts_with(':')) {
        opts = opts | di::drop(1);
        strict_mode = true;
    }
    auto print_errors = (opterr != 0) && !strict_mode;

    // Optind was reset, so reset the internal state.
    if (optind < 1) {
        optind = 1;
        next_short_char = nullptr;
    }

    // Handle any short options if present.
    if (auto result = handle_short_options(argc, argv, opts, strict_mode, print_errors); result.has_value()) {
        return result.value();
    }

    // Skip any positional arguments.
    auto original_optind = optind;
    auto skipped_count = 0;
    for (; optind < argc; optind++) {
        // If arg doesn't start with '-', or is exactly '-', then it is a positional argument.
        if (argv[optind][0] == '-' && argv[optind][1] != '\0') {
            break;
        }
        skipped_count++;
    }

    // Setup a roll-back to move any skipped arguments after any argument subsequently parsed.
    auto guard = di::ScopeExit([&] {
        if (skipped_count > 0) {
            auto args = di::Span { argv.begin() + original_optind, argv.begin() + optind };
            di::rotate(args, args.begin() + skipped_count);
            optind -= skipped_count;
        }
    });

    // If there are no more arguments, then return.
    if (optind >= argc) {
        return -1;
    }

    auto arg = di::TransparentStringView(argv[optind], strlen(argv[optind]));

    // "--" indicates every argument that follows it is positional, so return.
    if (arg == "--"_tsv) {
        optind++;
        return -1;
    }

    // Handle any long options if present.
    if (auto result = handle_long_options(argc, argv, arg, longopts, longindex, strict_mode, print_errors, long_only);
        result.has_value()) {
        return result.value();
    }

    // At this point, arg must be a short option.
    next_short_char = const_cast<char*>(arg.data()) + 1;
    return *handle_short_options(argc, argv, opts, strict_mode, print_errors);
}
}
