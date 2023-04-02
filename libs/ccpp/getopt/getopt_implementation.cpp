#include <ccpp/bits/getopt_implementation.h>
#include <di/prelude.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>

namespace ccpp {
static char* nextchar = nullptr;
static int first_non_opt = -1;

static bool is_short_opt(char const* str, bool short_only) {
    if (short_only) {
        return str[0] == '-' && strcmp(str, "-") != 0 && strcmp(str, "--") != 0;
    }
    return str[0] == '-' && str[1] != '\0' && str[1] != '-';
}

static bool is_long_opt(char const* str, bool short_only) {
    if (short_only) {
        return false;
    }
    return str[0] == '-' && str[1] == '-' && str[2] != '\0';
}

int getopt_implementation(int argc, char* const argv[], char const* optstring, const struct option* longopts,
                          int* longindex, bool long_only) {
    if (optstring[0] == '+' || optstring[0] == '-') {
        fprintf(stderr, "Getopt %c is not supported.\n", optstring[0]);
    }

    bool short_only = longopts != nullptr;
    bool strict_mode = false;
    if (optstring[0] == ':') {
        strict_mode = true;
    }

    bool show_errors = (opterr != 0) && !strict_mode;
    bool optional = false;
    bool requires_value = false;

    // Optind was reset, so reset this useless auxillary variable ???
    if (optind < first_non_opt) {
        first_non_opt = -1;
    }

    while (optind < argc) {
        // "--" indicates every argument the follows it is positional, so just return -1.
        if (strcmp(argv[optind], "--") == 0) {
            optind++;
            return -1;
        }

        if (!long_only && is_short_opt(argv[optind], short_only)) {
            nextchar = nextchar ? nextchar : (argv[optind] + 1);
            char first = *nextchar;
            char* opt_specifier = strchr(optstring, first);

            // Argument specifier does not exist
            if (opt_specifier == nullptr) {
                if (show_errors) {
                    fprintf(stderr, "Unknown option: `%c'\n", first);
                }

                optopt = first;
                goto setup_next_call;
            }

            optional = opt_specifier[1] == ':' && opt_specifier[2] == ':';
            requires_value = !optional && opt_specifier[1] == ':';

            if (optional || requires_value) {
                char* value = nextchar + 1;
                if (value[0] == '\0') {
                    if (optional) {
                        optarg = nullptr;
                        goto consume_arg_and_return;
                    }

                    /* Look at next argv if not optional */
                    value = argv[++optind];
                    if (value == nullptr) {
                        if (show_errors) {
                            fprintf(stderr, "Option `%c' requires an argument, but none was specified\n", first);
                        }

                        optopt = first;

                        nextchar = nullptr;
                        return strict_mode ? ':' : '?';
                    }
                }

                optarg = value;

            consume_arg_and_return:
                optind++;
                nextchar = nullptr;
                return first;
            }

        setup_next_call:
            nextchar++;
            if (*nextchar == '\0') {
                optind++;
                nextchar = nullptr;
            }

            return opt_specifier ? first : '?';
        }

        if (is_long_opt(argv[optind], short_only)) {
            char* name = argv[optind] + 2;

            char* value = strchr(name, '=');
            if (value) {
                value++;
            }

            size_t name_len = value ? (size_t) (value - name - 1) : strlen(name);

            const struct option* opt = nullptr;
            const struct option null_opt = { 0, 0, 0, 0 };
            if (longopts) {
                for (const struct option* iter = longopts; memcmp(iter, &null_opt, sizeof(*iter)) != 0; iter++) {
                    if (strncmp(name, iter->name, name_len) == 0) {
                        opt = iter;
                        break;
                    }
                }
            }

            if (longindex) {
                *longindex = opt - longopts;
            }

            if (!opt) {
                if (show_errors) {
                    fprintf(stderr, "Unknown option: `%s'\n", argv[optind]);
                }

                optind++;
                return '?';
            }

            if (opt->has_arg == no_argument && value) {
                if (show_errors) {
                    fprintf(stderr, "Option `%s' should not have a value\n", argv[optind]);
                }

                optind++;
                return '?';
            }

            if (opt->has_arg == required_argument && !value) {
                value = argv[++optind];
                if (!value) {
                    if (show_errors) {
                        fprintf(stderr, "Option `%s' requires an argument, but none was specified\n", name);
                    }

                    return strict_mode ? ':' : '?';
                }
            }

            int val = opt->val;
            if (opt->flag) {
                *opt->flag = val;
                val = 0;
            }
            optarg = value;
            optind++;
            return val;
        }

        if (first_non_opt != -1) {
            optarg = argv[++optind];
            return -1;
        }

        // First check if there is no non options left or else we could shift things forever
        bool all_not_options = true;
        for (int i = optind; i < argc; i++) {
            if ((!long_only && is_short_opt(argv[i], short_only)) || is_long_opt(argv[i], short_only)) {
                all_not_options = false;
                break;
            }
        }

        if (all_not_options) {
            optarg = argv[optind];
            first_non_opt = optind;
            return -1;
        }

        int next_arg = -1;
        for (int i = optind + 1; i < argc; i++) {
            if ((!long_only && is_short_opt(argv[i], short_only)) || (is_long_opt(argv[i], short_only))) {
                next_arg = i;
                break;
            }
        }

        ASSERT(next_arg != -1);
        char* temp = ((char**) argv)[next_arg];
        for (int i = next_arg; i > optind; i--) {
            ((char**) argv)[i] = ((char**) argv)[i - 1];
        }
        ((char**) argv)[optind] = temp;
    }

    return -1;
}
}
