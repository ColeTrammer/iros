#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s <file>\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, ":")) != -1) {
        switch (opt) {
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if (argc - optind != 1) {
        print_usage_and_exit(*argv);
    }

    char buf[PATH_MAX];
    ssize_t path_len;
    if ((path_len = readlink(argv[optind], buf, sizeof(buf))) == -1) {
        perror("realpath");
        return 1;
    }

    buf[path_len] = '\0';
    puts(buf);
    return 0;
}
