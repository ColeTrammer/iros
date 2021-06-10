#include <assert.h>
#include <bits/do_getopt.h>
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

char *optarg = NULL;
int optind = 1, opterr = 1, optopt = '\0';

static char *nextchar = NULL;
static int first_non_opt = -1;

static bool is_short_opt(const char *str, enum getopt_mode mode) {
    if (mode == SHORT) {
        return str[0] == '-' && strcmp(str, "-") != 0 && strcmp(str, "--") != 0;
    }
    return str[0] == '-' && str[1] != '\0' && str[1] != '-';
}

static bool is_long_opt(const char *str, enum getopt_mode mode) {
    if (mode == SHORT) {
        return false;
    }
    return str[0] == '-' && str[1] == '-' && str[2] != '\0';
}

int __do_getopt(int argc, char *const argv[], const char *optstring, const struct option *longopts, int *longindex, enum getopt_mode mode) {
    if (optstring[0] == '+' || optstring[0] == '-') {
        fprintf(stderr, "Getopt %c is not supported.\n", optstring[0]);
    }

    bool strict_mode = false;
    if (optstring[0] == ':') {
        strict_mode = true;
    }

    bool show_errors = (opterr != 0) && !strict_mode;

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

        if (is_short_opt(argv[optind], mode)) {
            nextchar = nextchar ? nextchar : (argv[optind] + 1);
            char first = *nextchar;
            char *opt_specifier = strchr(optstring, first);

            // Argument specifier does not exist
            if (opt_specifier == NULL) {
                if (show_errors) {
                    fprintf(stderr, "Unknown option: `%c'\n", first);
                }

                optopt = first;
                goto setup_next_call;
            }

            bool optional = opt_specifier[1] == ':' && opt_specifier[2] == ':';
            bool requires_value = !optional && opt_specifier[1] == ':';

            if (optional || requires_value) {
                char *value = nextchar + 1;
                if (value[0] == '\0') {
                    if (optional) {
                        optarg = NULL;
                        goto consume_arg_and_return;
                    }

                    /* Look at next argv if not optional */
                    value = argv[++optind];
                    if (value == NULL) {
                        if (show_errors) {
                            fprintf(stderr, "Option `%c' requires an argument, but none was specified\n", first);
                        }

                        optopt = first;

                        nextchar = NULL;
                        return strict_mode ? ':' : '?';
                    }
                }

                optarg = value;

            consume_arg_and_return:
                optind++;
                nextchar = NULL;
                return first;
            }

        setup_next_call:
            nextchar++;
            if (*nextchar == '\0') {
                optind++;
                nextchar = NULL;
            }

            return opt_specifier ? first : '?';
        }

        if (is_long_opt(argv[optind], mode)) {
            char *name = argv[optind] + 2;

            char *value = strchr(name, '=');
            if (value) {
                value++;
            }

            size_t name_len = value ? (size_t)(value - name - 1) : strlen(name);

            const struct option *opt = NULL;
            const struct option null_opt = { 0 };
            if (longopts) {
                for (const struct option *iter = longopts; memcmp(iter, &null_opt, sizeof(*iter)) != 0; iter++) {
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
            if (is_short_opt(argv[i], mode) || is_long_opt(argv[i], mode)) {
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
        for (int i = optind + 1; i < argc - 1; i++) {
            if (is_short_opt(argv[i], mode) || (is_long_opt(argv[i], mode))) {
                next_arg = i;
                break;
            }
        }

        assert(next_arg != -1);
        char *temp = ((char **) argv)[next_arg];
        for (int i = next_arg; i > optind; i--) {
            ((char **) argv)[i] = ((char **) argv)[i - 1];
        }
        ((char **) argv)[optind] = temp;
    }

    return -1;
}
