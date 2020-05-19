#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s\n", s);
    exit(2);
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

    if (optind != argc) {
        print_usage_and_exit(*argv);
    }

    char *name = ttyname(STDIN_FILENO);
    if (!name) {
        if (errno != ENOTTY) {
            perror("tty");
            return 2;
        }

        puts("not a tty");
        return 1;
    }

    printf("%s\n", name);
    return 0;
}
