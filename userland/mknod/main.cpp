#include <assert.h>
#include <errno.h>
#include <ext/parse_mode.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-m mode] name type [major minor]\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    int opt;
    mode_t umask_value = umask(0);
    mode_t mode = 0666 & ~umask_value;
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

    mode &= 07777;
    dev_t major = 0;
    dev_t minor = 0;
    if (argc - optind < 2) {
        print_usage_and_exit(*argv);
    }

    const char *path = argv[optind];
    const char *type = argv[optind + 1];
    if (strcmp(type, "b") == 0) {
        mode |= S_IFBLK;
    } else if (strcmp(type, "c") == 0 || strcmp(type, "u") == 0) {
        mode |= S_IFCHR;
    } else if (strcmp(type, "p") == 0) {
        mode |= S_IFIFO;
    } else {
        fprintf(stderr, "%s: unknown type: %s\n", *argv, type);
        return 2;
    }

    if ((S_ISFIFO(mode) && argc != optind + 2) || (!S_ISFIFO(mode) && argc != optind + 4)) {
        print_usage_and_exit(*argv);
    }

    if (!S_ISFIFO(mode)) {
        char *end_ptr;
        char *major_s = argv[optind + 2];
        char *minor_s = argv[optind + 3];

        errno = 0;
        major = strtol(major_s, &end_ptr, 0);
        if (errno) {
            fprintf(stderr, "%s: failed to read major device number `%s': %s\n", *argv, major_s, strerror(errno));
            return 1;
        }
        if (*end_ptr) {
            fprintf(stderr, "%s: invalid major device number: %s\n", *argv, major_s);
            return 1;
        }

        errno = 0;
        minor = strtol(minor_s, &end_ptr, 0);
        if (errno) {
            fprintf(stderr, "%s: failed to read minor device number `%s': %s\n", *argv, minor_s, strerror(errno));
            return 1;
        }
        if (*end_ptr) {
            fprintf(stderr, "%s: invalid minor device number: %s\n", *argv, minor_s);
            return 1;
        }
    }

    if (mknod(path, mode, ((major & 0xFFF) << 8) | (minor & 0xFF))) {
        fprintf(stderr, "%s: mknod: %s\n", *argv, strerror(errno));
        return 1;
    }

    return 0;
}
