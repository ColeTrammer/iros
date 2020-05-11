#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

char *optarg = NULL;
int optind = 1, opterr = 1, optopt = '\0';

static char *nextchar = NULL;
static int first_non_opt = -1;

static bool is_opt(char *str) {
    return str[0] == '-' && strcmp(str, "-") != 0 && strcmp(str, "--") != 0;
}

int getopt(int argc, char *const argv[], const char *optstring) {
    if (optstring[0] == '+' || optstring[0] == '-') {
        fprintf(stderr, "Getopt %c is not supported.\n", optstring[0]);
    }

    bool suppress_errors = false;
    if (optstring[0] == ':') {
        suppress_errors = true;
    }

    /* Somewhat modified optind */
    if (optind < first_non_opt) {
        first_non_opt = -1;
    }

    while (optind < argc) {
        if (is_opt(argv[optind])) {
            nextchar = nextchar ? nextchar : (argv[optind] + 1);
            char first = *nextchar;
            char *opt_specifier = strchr(optstring, first);

            /* Specifier not found */
            if (opt_specifier == NULL) {
                if (opterr && !suppress_errors) {
                    fprintf(stderr, "Invalid option: %c\n", first);
                }

                optopt = first;
                goto setup_next_call;
            }

            bool optional = opt_specifier[1] == ':' && opt_specifier[2] == ':';
            bool has_value = !optional && opt_specifier[1] == ':';

            if (optional || has_value) {
                char *value = nextchar + 1;
                if (value[0] == '\0') {
                    if (optional) {
                        optarg = NULL;
                        goto consume_arg_and_return;
                    }

                    /* Look at next argv if not optional */
                    value = argv[++optind];
                    if (value == NULL || is_opt(value)) {
                        if (opterr && !suppress_errors) {
                            fprintf(stderr, "No value specified for option: %c\n", first);
                        }

                        optopt = first;

                        /* Since we read the next argv and it was an option, read it next time */
                        nextchar = NULL;
                        return suppress_errors ? ':' : '?';
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

        if (first_non_opt != -1) {
            optarg = argv[++optind];
            break;
        }

        /* First check if there is no non options left or else we could shift things forever */
        bool all_not_options = true;
        for (int i = optind; i < argc; i++) {
            if (is_opt(argv[i])) {
                all_not_options = false;
                break;
            }
        }

        if (all_not_options) {
            optarg = argv[optind];
            first_non_opt = optind;
            break;
        }

        char *temp = argv[optind];
        for (int i = optind; i < argc - 1; i++) {
            ((char **) argv)[i] = argv[i + 1];
        }

        ((char **) argv)[argc - 1] = temp;
    }

    return -1;
}
