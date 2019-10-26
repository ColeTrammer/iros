#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int process_file(FILE *file, char *pattern, size_t pattern_len, char *path, bool print_path) {
    char *line = NULL;
    size_t line_max = 0;

    char *path_color = isatty(STDOUT_FILENO) ? "\033[35m" : "";
    char *found_color = isatty(STDOUT_FILENO) ? "\033[31m" : "";
    char *restore_color = isatty(STDOUT_FILENO) ? "\033[0m" : "";

    while (getline(&line, &line_max, file) != -1) {
        char *end_line = strchr(line, '\n');
        if (end_line) {
            *end_line = '\0';
        }

        char *match = NULL;
        char *s = line;
        bool matched = false;
        while ((match = strstr(s, pattern))) {
            *match = '\0';
            if (!matched && print_path) {
                printf("%s%s%s:", path_color, path, restore_color);
            }
            printf("%s%s%s%s", s, found_color, pattern, restore_color);
            s = match + pattern_len;
            matched = true;
        }

        if (!matched) {
            continue;
        }

        printf("%s\n", s);
    }

    free(line);
    return 0;
}

void print_usage(char **argv) {
    printf("Usage: %s [-r] <pattern> <files ...>\n", argv[0]);
}

int main(int argc, char **argv) {
    if (argc == 1) {
        print_usage(argv);
        return 0;
    }

    bool recursive = false;
    (void) recursive;
    char opt;

    opterr = 0;
    while ((opt = getopt(argc, argv, ":r")) != -1) {
        switch (opt) {
            case 'l':
                recursive = true;
                break;
            case '?':
                print_usage(argv);
                return 0;
            default:
                abort();
        }
    }

    if (optind >= argc) {
        print_usage(argv);
        return 0;
    }

    char *pattern = argv[optind++];
    size_t pattern_len = strlen(pattern);

    bool print_path = argc - optind >= 2;

    if (optind >= argc) {
        return process_file(stdin, pattern, pattern_len, "(standard input)", print_path);
    }

    int status = 0;
    while (optind < argc) {
        char *path = argv[optind++];
        int ret = 0;
        if (strcmp(path, "-") == 0) {
            ret = process_file(stdin, pattern, pattern_len, "(standard input)", print_path);
        } else {
            FILE *file = fopen(path, "r");
            if (!file) {
                perror("grep");
                ret = 1;
                goto next;
            }

            ret = process_file(file, pattern, pattern_len, path, print_path);
            if (ret != 0) {
                goto next;
            }

            if (fclose(file)) {
                perror("grep");
                ret = 1;
            }
        }

    next:
        if (ret != 0) {
            status = ret;
        }
    }

    return status;
}