#include <ftw.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *pattern = NULL;
static size_t pattern_len = 0;
static bool print_path = false;
static int status = 0;

static int process_file(FILE *file, char *pattern, size_t pattern_len, const char *path, bool print_path) {
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

static int process_and_read_file(const char *path, char *pattern, size_t pattern_len, bool print_path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        perror("grep");
        return 1;
    }

    int ret = process_file(file, pattern, pattern_len, path, print_path);
    if (ret != 0) {
        return ret;
    }

    if (fclose(file)) {
        perror("grep");
        ret = 1;
    }

    return ret;
}

static int per_path(const char *path, const struct stat *stat_struct, int type, struct FTW *ftwbuf) {
    (void) stat_struct;
    (void) ftwbuf;

    if (type == FTW_F) {
        int ret = process_and_read_file(path, pattern, pattern_len, print_path);
        if (ret != 0) {
            status = ret;
        }
    }

    return 0; // Continue
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
            case 'r':
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

    pattern = argv[optind++];
    pattern_len = strlen(pattern);
    print_path = argc - optind >= 2 || recursive;

    if (optind >= argc) {
        if (!recursive) {
            return process_file(stdin, pattern, pattern_len, "(standard input)", print_path);
        } else {
            if (nftw(".", per_path, FOPEN_MAX - 4, 0)) {
                perror("grep");
                return 1;
            }
            return status;
        }
    }

    while (optind < argc) {
        char *path = argv[optind++];
        int ret = 0;
        if (strcmp(path, "-") == 0) {
            ret = process_file(stdin, pattern, pattern_len, "(standard input)", print_path);
        } else if (!recursive) {
            ret = process_and_read_file(path, pattern, pattern_len, print_path);
        } else {
            if (nftw(path, per_path, FOPEN_MAX - 4, 0)) {
                perror("grep");
                status = 1;
            }
            continue;
        }

        if (ret != 0) {
            status = ret;
        }
    }

    return status;
}