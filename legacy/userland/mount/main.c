#include <stdio.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <unistd.h>

static void print_usage_and_exit(const char *name) {
    fprintf(stderr, "Usage: %s [-r] -t type <source> <target>\n", name);
    exit(1);
}

int main(int argc, char **argv) {
    char *fs_type = NULL;
    unsigned long flags = 0;

    int opt;
    while ((opt = getopt(argc, argv, ":rt:")) != -1) {
        switch (opt) {
            case 'r':
                flags |= MS_RDONLY;
                break;
            case 't':
                fs_type = optarg;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
        }
    }

    if (!fs_type || optind != argc - 2) {
        print_usage_and_exit(*argv);
    }

    if (mount(argv[optind], argv[optind + 1], fs_type, flags, NULL)) {
        perror("mount");
        return 1;
    }

    return 0;
}
