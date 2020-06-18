#include <errno.h>
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
    mode_t mode = 0666;

    int opt;
    while ((opt = getopt(argc, argv, ":m:")) != -1) {
        switch (opt) {
            case 'm': {
                char *end_ptr;
                errno = 0;
                mode = strtol(optarg, &end_ptr, 8);
                if (errno) {
                    fprintf(stderr, "%s: failed to read mode `%s': %s\n", *argv, optarg, strerror(errno));
                    return 1;
                }
                if (*end_ptr) {
                    fprintf(stderr, "%s: invalid mode: %s\n", *argv, optarg);
                    return 1;
                }
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