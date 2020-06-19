#include <errno.h>
#include <ext/parse_mode.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-m mode] <names...>\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    mode_t umask_value = umask(0);
    mode_t mode = 0666 & ~umask_value;

    int opt;
    while ((opt = getopt(argc, argv, ":m:")) != -1) {
        switch (opt) {
            case 'm': {
                auto fancy_mode = Ext::parse_mode(optarg);
                if (!fancy_mode.has_value()) {
                    fprintf(stderr, "%s: failed to parse mode: `%s'\n", *argv, optarg);
                    return 1;
                }
                mode = fancy_mode.value().resolve(mode, umask_value);
                break;
            }
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if (optind == argc) {
        print_usage_and_exit(*argv);
    }

    bool any_failed = false;
    for (; optind < argc; optind++) {
        if (mkfifo(argv[optind], mode)) {
            fprintf(stderr, "%s: Failed to create `%s': %s\n", *argv, argv[optind], strerror(errno));
            any_failed = true;
        }
    }

    return any_failed;
}