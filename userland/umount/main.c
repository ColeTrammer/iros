#include <stdio.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <unistd.h>

static void print_usage_and_exit(const char *name) {
    fprintf(stderr, "Usage: %s <path>\n", name);
    exit(1);
}

int main(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, ":")) != -1) {
        switch (opt) {
            case ':':
            case '?':
                print_usage_and_exit(*argv);
        }
    }

    if (optind != argc - 1) {
        print_usage_and_exit(*argv);
    }

    if (umount(argv[optind])) {
        perror("umount");
        return 1;
    }

    return 0;
}
