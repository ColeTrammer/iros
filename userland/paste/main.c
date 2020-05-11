#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char get_delim_for_index(const char *delim, size_t delim_length, size_t index) {
    return delim[index % delim_length];
}

int do_paste(FILE **files, size_t num_files, const char *delim, size_t delim_length) {
    char **lines = calloc(num_files, sizeof(char *));
    size_t *line_maxes = calloc(num_files, sizeof(char *));
    bool *is_eof = calloc(num_files, sizeof(bool));

    size_t eof_count = 0;
    int ret = 0;
    for (;;) {
        for (size_t i = 0; i < num_files; i++) {
            if (is_eof[i]) {
                continue;
            }

            ssize_t ret = getline(&lines[i], &line_maxes[i], files[i]);
            if (ret == -1) {
                if (ferror(files[i])) {
                    perror("paste: getline");
                    ret = 1;
                    goto end_paste;
                }

                free(lines[i]);
                lines[i] = NULL;

                // For repeated stdin
                clearerr(files[i]);
                is_eof[i] = true;
                eof_count++;
                continue;
            }

            char *trailing_newline = strchr(lines[i], '\n');
            if (trailing_newline) {
                *trailing_newline = '\0';
            }
        }

        if (eof_count >= num_files) {
            break;
        }

        for (size_t i = 0; i < num_files; i++) {
            const char *line = lines[i];
            if (line) {
                fputs(line, stdout);
            }

            if (i == num_files - 1) {
                putchar('\n');
            } else {
                char delim_char = get_delim_for_index(delim, delim_length, i);
                if (delim_char != '\0') {
                    putchar(get_delim_for_index(delim, delim_length, i));
                }
            }
        }
    }

end_paste:
    for (size_t i = 0; i < num_files; i++) {
        free(lines[i]);
    }
    free(lines);
    free(line_maxes);
    free(is_eof);
    return ret;
}

int do_serial_paste(FILE **files, size_t num_files, const char *delim, size_t delim_length) {
    char *line = NULL;
    size_t line_max = 0;

    for (size_t i = 0; i < num_files; i++) {
        size_t line_number = 0;
        while (getline(&line, &line_max, files[i]) != -1) {
            if (line_number != 0) {
                char delim_char = get_delim_for_index(delim, delim_length, line_number - 1);
                if (delim_char != '\0') {
                    putchar(delim_char);
                }
            }

            char *trailing_newline = strchr(line, '\n');
            if (trailing_newline) {
                *trailing_newline = '\0';
            }
            fputs(line, stdout);
            line_number++;
        }

        if (ferror(files[i])) {
            perror("paste: getline");
            return 1;
        }

        // For repeated stdin's
        clearerr(files[i]);
        putchar('\n');
    }

    free(line);
    return 0;
}

char *parse_delim(const char *input, size_t *out_length) {
    size_t length = 0;
    size_t max = 20;
    char *delim = malloc(max);

    bool prev_was_backslash = false;
    for (size_t i = 0; input[i] != '\0'; i++) {
        char c = input[i];
        switch (c) {
            case '\\':
                if (!prev_was_backslash) {
                    prev_was_backslash = true;
                    continue;
                }
                break;
            case 'n':
                if (prev_was_backslash) {
                    c = '\n';
                }
                break;
            case 't':
                if (prev_was_backslash) {
                    c = '\t';
                }
                break;
            case '0':
                if (prev_was_backslash) {
                    c = '\0';
                }
                break;
            default:
                break;
        }

        if (length >= max) {
            max *= 2;
            delim = realloc(delim, max);
        }
        delim[length++] = c;
        prev_was_backslash = false;
    }

    if (prev_was_backslash) {
        free(delim);
        return NULL;
    }

    delim[length] = '\0';
    *out_length = length;
    return delim;
}

void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-s] [-d list] [file...]\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    char *delim = "\t";
    bool serial = false;

    int opt;
    while ((opt = getopt(argc, argv, ":sd:")) != -1) {
        switch (opt) {
            case 's':
                serial = true;
                break;
            case 'd':
                delim = optarg;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    size_t real_delim_size = 0;
    char *real_delim = parse_delim(delim, &real_delim_size);
    if (!real_delim) {
        fprintf(stderr, "paste: failed to parse dilimeter: `%s'\n", delim);
        return 1;
    }

    if (optind == argc) {
        argv[argc++] = "-";
    }

    int ret = 0;

    size_t num_files = 0;
    size_t max_files = 20;
    FILE **files = malloc(max_files * sizeof(FILE *));
    for (int i = optind; i < argc; i++) {
        if (num_files >= max_files) {
            max_files *= 2;
            files = realloc(files, max_files * sizeof(FILE *));
        }

        char *path = argv[i];

        FILE *f;
        if (strcmp(path, "-") == 0) {
            f = stdin;
        } else {
            f = fopen(path, "r");
            if (!f) {
                perror("paste: fopen");
                ret = 1;
                goto end;
            }
        }

        files[num_files++] = f;
    }

    if (serial) {
        ret = do_serial_paste(files, num_files, real_delim, real_delim_size);
    } else {
        ret = do_paste(files, num_files, real_delim, real_delim_size);
    }

end:
    for (size_t i = 0; i < num_files; i++) {
        if (files[i] != stdin && fclose(files[i])) {
            perror("paste: fclose");
            ret = 1;
        }
    }
    free(real_delim);
    return ret;
}
