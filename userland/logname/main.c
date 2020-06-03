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

    char *logname = getlogin();
    if (!logname) {
        fprintf(stderr, "%s: no login information available\n", *argv);
        return 1;
    }

    printf("%s\n", logname);
    return 0;
}
