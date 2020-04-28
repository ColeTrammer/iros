#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

enum cut_mode { MODE_INVALID, MODE_BYTE, MODE_CHAR, MODE_FIELD };

struct cut_range {
    long low;
    long high;
};

struct cut_info {
    enum cut_mode mode;
    bool dont_split_chars;
    bool suppress_delimiterless_lines;
    char delim;
    struct cut_range *criteria;
    size_t criteria_length;
};

bool cut_matches(long position, struct cut_range *criteria, size_t criteria_length) {
    for (size_t i = 0; i < criteria_length; i++) {
        struct cut_range range = criteria[i];
        if (position >= range.low && position <= range.high) {
            return true;
        }
    }

    return false;
}

void do_line_cut(const char *line, struct cut_info *info) {
    switch (info->mode) {
        case MODE_INVALID:
            assert(false);
        case MODE_CHAR:
        case MODE_BYTE:
            // FIXME: support the -n option in byte mode
            //        right now the os doesn't even support mulitbyte characters,
            //        but if it ever does, the -n option should actually do something.
            for (long i = 0; line[i] != '\0'; i++) {
                if (cut_matches(i + 1, info->criteria, info->criteria_length)) {
                    putchar(line[i]);
                }
            }
            break;
        case MODE_FIELD:
            break;
    }

    putchar('\n');
}

int do_cut(FILE *input, struct cut_info *info) {
    char *line = NULL;
    size_t line_max = 0;
    ssize_t line_length;

    while ((line_length = getline(&line, &line_max, input)) != -1) {
        char *trailing_newline = strchr(line, '\n');
        if (trailing_newline) {
            *trailing_newline = '\0';
        }

        do_line_cut(line, info);
    }

    int ret = 0;
    if (ferror(input)) {
        ret = 1;
    }

    free(line);
    return ret;
}

int cut_parse_range(const char *str, struct cut_range *out_range) {
    size_t len = strlen(str);
    if (len <= 1) {
        return 1;
    }

    const char *end = str + len;
    char *parse_end = NULL;
    if (*str == '-') {
        long result = strtol(str + 1, &parse_end, 10);
        if (errno != 0) {
            return 1;
        }

        if (result <= 0) {
            return 1;
        }

        if (parse_end != end) {
            return 1;
        }
        out_range->low = 1;
        out_range->high = result;
        return 0;
    }

    long low = strtol(str, &parse_end, 10);
    if (errno != 0) {
        return 1;
    }

    if (low <= 0) {
        return 1;
    }

    if (*parse_end != '-') {
        return 1;
    }

    if (parse_end + 1 == end) {
        out_range->low = low;
        out_range->high = LONG_MAX;
        return 0;
    }

    long high = strtol(parse_end + 1, &parse_end, 10);
    if (errno != 0) {
        return 1;
    }

    if (high <= 1) {
        return 1;
    }

    if (low > high) {
        return 1;
    }

    if (parse_end != end) {
        return 1;
    }

    out_range->low = low;
    out_range->high = high;
    return 0;
}

struct cut_range *cut_parse_list(char *list, size_t *length) {
    if (!*list) {
        return NULL;
    }

    struct cut_range *criteria = NULL;
    *length = 0;
    size_t size = 0;

    char *part = strtok(list, ", ");
    while (part) {
        struct cut_range range;
        if (cut_parse_range(part, &range)) {
            free(criteria);
            fprintf(stderr, "cut: failed to parse range `%s'\n", part);
            return NULL;
        }

        if (*length >= size) {
            size = MAX(size * 2, 20);
            criteria = realloc(criteria, size * sizeof(struct cut_range));
            if (!criteria) {
                perror("cut: realloc");
                return NULL;
            }
        }

        criteria[(*length)++] = range;
        part = strtok(NULL, ", ");
    }

    return criteria;
}

void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-b|-c|-f list] [-n] [-d delim] [-s] [files...]\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    enum cut_mode mode = MODE_INVALID;
    const char *delim = NULL;
    char *list = NULL;
    (void) list;
    bool dont_split_chars = false;
    bool suppress_delimiterless_lines = false;

    int opt;
    while ((opt = getopt(argc, argv, ":b:c:f:nd:s")) != -1) {
        switch (opt) {
            case 'b':
                if (mode != MODE_INVALID) {
                    print_usage_and_exit(*argv);
                }
                mode = MODE_BYTE;
                list = optarg;
                break;
            case 'c':
                if (mode != MODE_INVALID) {
                    print_usage_and_exit(*argv);
                }
                mode = MODE_CHAR;
                list = optarg;
                break;
            case 'f':
                if (mode != MODE_INVALID) {
                    print_usage_and_exit(*argv);
                }
                mode = MODE_FIELD;
                list = optarg;
                break;
            case 'n':
                dont_split_chars = true;
                break;
            case 'd':
                delim = optarg;
                break;
            case 's':
                suppress_delimiterless_lines = true;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
        }
    }

    if (mode == MODE_INVALID) {
        print_usage_and_exit(*argv);
    }

    if ((dont_split_chars) && mode != MODE_BYTE) {
        print_usage_and_exit(*argv);
    }

    if ((delim || suppress_delimiterless_lines) && mode != MODE_FIELD) {
        print_usage_and_exit(*argv);
    }

    if (delim && strlen(delim) != 1) {
        fprintf(stderr, "cut: the delimiter must be a single character\n");
        return 2;
    }

    size_t criteria_length;
    struct cut_range *criteria = cut_parse_list(list, &criteria_length);
    if (!criteria) {
        return 2;
    }

    struct cut_info info = { .mode = mode,
                             .dont_split_chars = dont_split_chars,
                             .suppress_delimiterless_lines = suppress_delimiterless_lines,
                             .delim = !delim ? '\t' : *delim,
                             .criteria = criteria,
                             .criteria_length = criteria_length };

    bool any_failed = false;
    if (optind == argc) {
        argv[argc++] = "-";
    }

    for (; optind < argc; optind++) {
        const char *path = argv[optind];
        FILE *f = strcmp(path, "-") == 0 ? stdin : fopen(path, "r");
        if (!f) {
            perror("cut: fopen");
            any_failed = true;
            continue;
        }

        if (do_cut(f, &info)) {
            any_failed = true;
        }

        if (f == stdin) {
            // Clear EOF so that the user can specify '-' multiple times
            clearerr(stdin);
        } else if (fclose(f)) {
            perror("cut: fclose");
            any_failed = true;
        }
    }

    free(criteria);
    return any_failed;
}