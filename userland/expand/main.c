#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

static int do_expand(FILE *f) {
    int col_position = 0;
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == '\t') {
            int num_spaces = 8 - (col_position % 8);
            col_position += num_spaces;
            for (int i = 0; i < num_spaces; i++) {
                putchar(' ');
            }
            continue;
        } else if (c == 127) {
            col_position = MAX(col_position - 1, 0);
        } else if (c == '\n' || c == '\r') {
            col_position = 0;
        } else {
            col_position++;
        }
        putchar(c);
    }

    if (ferror(f)) {
        perror("expand: fgetc");
        return 1;
    }

    return 0;
}

static void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-t tablist] [file...]\n", s);
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

    if (optind == argc) {
        argv[argc++] = "-";
    }

    bool any_failed = false;
    for (; optind < argc; optind++) {
        FILE *f = stdin;
        if (strcmp(argv[optind], "-") != 0) {
            f = fopen(argv[optind], "r");
            if (!f) {
                perror("expand: fopen");
                any_failed = 1;
                continue;
            }
        }

        if (do_expand(f)) {
            any_failed = 1;
        }

        if (f == stdin) {
            clearerr(stdin);
        } else if (fclose(f)) {
            perror("expand: fclose");
            any_failed = 1;
        }
    }

    return any_failed;
}
