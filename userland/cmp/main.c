#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool list;
static bool suppress_output;
static FILE *a;
static FILE *b;

static void close_a(void) {
    if (fclose(a)) {
        perror("fclose");
    }
}

static void close_b(void) {
    if (fclose(b)) {
        perror("fclose");
    }
}

int do_cmp(const char *path1, const char *path2) {
    if (strcmp(path1, "-") == 0) {
        a = stdin;
    } else {
        a = fopen(path1, "r");
        if (!a) {
            return 2;
        }

        atexit(close_a);
    }

    if (strcmp(path2, "-") == 0) {
        b = stdin;
    } else {
        b = fopen(path2, "r");
        if (!b) {
            return 2;
        }

        atexit(close_b);
    }

    bool differ = false;
    int line_number = 1;
    int byte_count = 1;
    for (;;) {
        int c1 = fgetc(a);
        int c2 = fgetc(b);

        if (c1 == EOF && c2 != EOF) {
            if (ferror(a)) {
                perror("fgetc");
                return 2;
            }

            if (!suppress_output) {
                fprintf(stderr, "cmp: EOF on %s after byte %d\n", path1, byte_count - 1);
            }

            differ = true;
            break;
        }

        if (c2 == EOF && c1 != EOF) {
            if (ferror(b)) {
                perror("fgetc");
                return 2;
            }

            if (!suppress_output) {
                fprintf(stderr, "cmp: EOF on %s after byte %d\n", path2, byte_count - 1);
            }
            differ = true;
            break;
        }

        if (c1 == EOF && c2 == EOF) {
            if (ferror(a)) {
                perror("fgetc");
                return 2;
            }

            if (ferror(b)) {
                perror("fgetc");
                return 2;
            }
            break;
        }

        if (c1 != c2) {
            differ = true;

            if (!list) {
                if (!suppress_output) {
                    printf("%s %s differ: char %d, line %d\n", path1, path2, byte_count, line_number);
                }
                break;
            }

            printf("%d %o %o\n", byte_count, c1, c2);
        }

        if (c1 == '\n') {
            line_number++;
        }

        byte_count++;
    }

    return differ;
}

void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-l|-s] <file1> <file2>\n", s);
    exit(3);
}

int main(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, ":ls")) != -1) {
        switch (opt) {
            case 'l':
                list = true;
                break;
            case 's':
                suppress_output = true;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
        }
    }

    if (suppress_output && list) {
        print_usage_and_exit(*argv);
    }

    if (argc - optind != 2) {
        print_usage_and_exit(*argv);
    }

    return do_cmp(argv[optind], argv[optind + 1]);
}
