#include <unistd.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

char *optarg = NULL;
int optind = 1, opterr = 1, optopt = '\0';

static char *nextchar = NULL;

static bool is_opt(char *str) {
    return str[0] == '-' && strcmp(str, "-") != 0 && strcmp(str, "--") != 0;
}

int getopt(int argc, char *const argv[], const char *optstring) {
    while (optind < argc) {
        if (is_opt(argv[optind])) {
            char first = nextchar ? nextchar[0] : argv[optind][1];
            char *opt_specifier = strchr(optstring, first);

            /* Specifier not found */
            if (opt_specifier == NULL) {
                if (opterr) {
                    fprintf(stderr, "Invalid option: %c\n", opt_specifier[0]);
                    optopt = opt_specifier[0];
                }

                goto setup_next_call;
            }

            // bool optional = opt_specifier[1] == ':' && opt_specifier[2] == ':';
            bool has_value = /* !optional && */ opt_specifier[1] == ':';

            if (has_value) {
                char *value = argv[optind] + 1;
                /* Look at next argv */
                if (value[0] == '\0') {
                    value = argv[++optind];
                    if (value == NULL) {
                        if (opterr) {
                            fprintf(stderr, "No value specified for option: %c\n", opt_specifier[0]);
                            optopt = opt_specifier[0];
                        }
                        
                        /* Since we're at the end of the args, just return ? and never look again */
                        optind = argc;
                        return '?';
                    }
                }

                optarg = value;
                optind++;
                nextchar = NULL;
                return opt_specifier[0];
            }

        setup_next_call:
            nextchar++;
            if (*nextchar == '\0') {
                optind++;
                nextchar = NULL;
            }

            return opt_specifier ? opt_specifier[0] : '?';
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
            optarg = argv[optind++];
            break;
        }

        for (int i = argc - 1; i > optind; i--) {
            ((char**) argv)[i - 1] = argv[i];
        }

        ((char**) argv)[argc - 1] = argv[optind];
        ((char**) argv)[optind] = argv[optind + 1];
    }

    return -1;
}