#include <ftw.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

static char *pattern = NULL;
static size_t pattern_len = 0;
static bool print_path = false;
static int status = 1;
static regex_t regex;
static bool use_regex = true;

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
        if (!use_regex) {
            while ((match = strstr(s, pattern))) {
                if (!matched && print_path) {
                    printf("%s%s%s:", path_color, path, restore_color);
                }

                *match = '\0';
                printf("%s%s%s%s", s, found_color, pattern, restore_color);
                s = match + pattern_len;
                matched = true;
                status = 0;
            }
        } else {
            regmatch_t regmatch;
            while (regexec(&regex, s, 1, &regmatch, matched ? REG_NOTBOL : 0) == 0) {
                if (!matched && print_path) {
                    printf("%s%s%s:", path_color, path, restore_color);
                }

                char start_save = s[regmatch.rm_so];
                s[regmatch.rm_so] = '\0';
                printf("%s", s);
                s[regmatch.rm_so] = start_save;

                char end_save = s[regmatch.rm_eo];
                s[regmatch.rm_eo] = '\0';
                printf("%s%s%s", found_color, s + regmatch.rm_so, restore_color);
                s[regmatch.rm_eo] = end_save;

                s += MAX(regmatch.rm_eo, 1);
                matched = true;
                status = 0;
            }
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
    printf("Usage: %s [-r] [-Ei|F] <pattern> <files ...>\n", argv[0]);
}

int main(int argc, char **argv) {
    if (argc == 1) {
        print_usage(argv);
        return 0;
    }

    bool recursive = strcmp(argv[0], "rgrep") == 0;
    int regex_flags = REG_NEWLINE | (strcmp(argv[0], "egrep") == 0 ? REG_EXTENDED : 0);
    use_regex = strcmp(argv[0], "fgrep") != 0;

    char opt;
    opterr = 0;
    while ((opt = getopt(argc, argv, ":rEFi")) != -1) {
        switch (opt) {
            case 'r':
                recursive = true;
                break;
            case 'E':
                regex_flags |= REG_EXTENDED;
                break;
            case 'F':
                use_regex = false;
                break;
            case 'i':
                regex_flags |= REG_ICASE;
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
    if (use_regex) {
        int ret = regcomp(&regex, pattern, regex_flags);
        if (ret != 0) {
            char buf[512];
            regerror(ret, &regex, buf, 512);
            fprintf(stderr, "grep: regex failed: %s\n", pattern);
            return 3;
        }
    }

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