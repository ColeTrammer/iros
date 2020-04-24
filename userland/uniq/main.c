#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool suppress_duplicates;
static bool suppress_nonduplicates;
static bool show_count;

static int do_uniq(FILE *in_file, FILE *out_file) {
    char *prev_line = NULL;
    char *line = NULL;
    int duplicate_count = 0;
    size_t line_max = 0;
    ssize_t line_length;
    while ((line_length = getline(&line, &line_max, in_file)) > 0) {
        if (strrchr(line, '\n')) {
            *strrchr(line, '\n') = '\0';
        }

        if (!prev_line) {
            prev_line = strdup(line);
            continue;
        }

        if (strcmp(prev_line, line) == 0) {
            duplicate_count++;
            continue;
        }

        if (!((suppress_duplicates && duplicate_count != 0) || (suppress_nonduplicates && duplicate_count == 0))) {
            if (show_count) {
                fprintf(out_file, "%7d ", duplicate_count + 1);
            }
            fprintf(out_file, "%s\n", prev_line);
        }

        free(prev_line);
        prev_line = strdup(line);
        duplicate_count = 0;
    }

    if (prev_line && !((suppress_duplicates && duplicate_count != 0) || (suppress_nonduplicates && duplicate_count == 0))) {
        if (show_count) {
            fprintf(out_file, "%7d ", duplicate_count + 1);
        }
        fprintf(out_file, "%s\n", prev_line);
    }

    free(prev_line);

    if (ferror(in_file)) {
        perror("getline");
        return 1;
    }

    return 0;
}

void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-cdu] [input_file [output_file]]\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, ":cdu")) != -1) {
        switch (opt) {
            case 'c':
                show_count = true;
                break;
            case 'd':
                suppress_nonduplicates = true;
                break;
            case 'u':
                suppress_duplicates = true;
                break;
            case '?':
            case ':':
                print_usage_and_exit(*argv);
        }
    }

    const char *in_file_path = "-";
    FILE *out_file = stdout;

    if (argc - optind > 2) {
        print_usage_and_exit(*argv);
    }

    if (argc - optind == 2) {
        out_file = fopen(argv[optind + 1], "w");
        if (!out_file) {
            perror("fopen");
            return 1;
        }
    }

    if (argc - optind > 0) {
        in_file_path = argv[optind];
    }

    FILE *in_file;
    if (strcmp(in_file_path, "-") == 0) {
        in_file = stdin;
    } else {
        in_file = fopen(in_file_path, "r");
        if (!in_file) {
            perror("fopen");
            return 1;
        }
    }

    int result = do_uniq(in_file, out_file);
    if (fclose(in_file)) {
        perror("fclose");
        result = 1;
    }

    if (fclose(out_file)) {
        perror("fclose");
        result = 1;
    }

    return result;
}