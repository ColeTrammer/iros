#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool suppress_1;
static bool suppress_2;
static bool suppress_3;

static FILE *files[2];
static size_t file_index;

static void close_files(void) {
    for (size_t i = 0; i < file_index; i++) {
        if (fclose(files[i])) {
            perror("fclose");
        }
    }
}

static FILE *comm_open(const char *path) {
    if (strcmp(path, "-") == 0) {
        return stdin;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen");
        exit(1);
    }

    return files[file_index++] = f;
}

static void output_col1(const char *s) {
    if (!suppress_1) {
        printf("%s\n", s);
    }
}

static void output_col2(const char *s) {
    if (!suppress_2) {
        printf("%s%s\n", suppress_1 ? "" : "\t", s);
    }
}

static void output_col3(const char *s) {
    if (!suppress_3) {
        printf("%s%s%s\n", suppress_1 ? "" : "\t", suppress_2 ? "" : "\t", s);
    }
}

#define getline_strip(lp, m, f)                     \
    ({                                              \
        ssize_t __result = getline((lp), (m), (f)); \
        if (__result > 0 && strchr(*(lp), '\n')) {  \
            *strchr(*(lp), '\n') = '\0';            \
        }                                           \
        __result;                                   \
    })

static int do_comm(const char *p1, const char *p2) {
    FILE *a = comm_open(p1);
    FILE *b = comm_open(p2);

    char *line_a = NULL;
    size_t line_a_max = 0;

    char *line_b = NULL;
    size_t line_b_max = 0;

    ssize_t line_a_length = getline_strip(&line_a, &line_a_max, a);
    ssize_t line_b_length = getline_strip(&line_b, &line_b_max, b);

    while (line_a_length != -1 || line_b_length != -1) {
        if (line_b_length == -1) {
            output_col1(line_a);
            line_a_length = getline_strip(&line_a, &line_a_max, a);
            continue;
        }

        if (line_a_length == -1) {
            output_col2(line_b);
            line_b_length = getline_strip(&line_b, &line_b_max, b);
            continue;
        }

        int compare_result = strcmp(line_a, line_b);
        if (compare_result == 0) {
            output_col3(line_a);
            line_a_length = getline_strip(&line_a, &line_a_max, a);
            line_b_length = getline_strip(&line_b, &line_b_max, b);
            continue;
        }

        if (compare_result < 0) {
            output_col1(line_a);
            line_a_length = getline_strip(&line_a, &line_a_max, a);
            continue;
        }

        if (compare_result > 0) {
            output_col2(line_b);
            line_b_length = getline_strip(&line_b, &line_b_max, b);
            continue;
        }
    }

    if (ferror(a)) {
        perror("getline");
        return 1;
    }

    if (ferror(b)) {
        perror("getline");
        return 1;
    }

    return 0;
}

void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-123] <file1> <file2>\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, ":123")) != -1) {
        switch (opt) {
            case '1':
                suppress_1 = true;
                break;
            case '2':
                suppress_2 = true;
                break;
            case '3':
                suppress_3 = true;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
        }
    }

    if (argc - optind != 2) {
        print_usage_and_exit(*argv);
    }

    atexit(close_files);
    return do_comm(argv[optind], argv[optind + 1]);
}
