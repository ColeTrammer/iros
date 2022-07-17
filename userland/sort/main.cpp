#include <liim/container/algorithm/sort.h>
#include <liim/string.h>
#include <liim/vector.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int read_all_path(const char* path, Vector<String>& lines) {
    FILE* file;
    if (strcmp(path, "-") == 0) {
        file = stdin;
    } else {
        file = fopen(path, "r");
        if (!file) {
            perror("sort: fopen");
            return 1;
        }
    }

    char* line = nullptr;
    size_t line_max = 0;
    ssize_t line_length;
    while ((line_length = getline(&line, &line_max, file)) != -1) {
        char* trailing_newline = strchr(line, '\n');
        if (trailing_newline) {
            *trailing_newline = '\0';
        }

        lines.add(String(line));
    }

    free(line);

    int ret = 0;
    if (ferror(file)) {
        perror("sort: getline");
        ret = 1;
    }

    if (file == stdin) {
        clearerr(stdin);
    } else if (fclose(file)) {
        perror("sort: fclose");
        ret = 1;
    }

    return ret;
}

void print_usage_and_exit(const char* s) {
    fprintf(stderr, "Usage: %s [files...]\n", s);
    exit(3);
}

int main(int argc, char** argv) {
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
        argv[argc++] = const_cast<char*>("-");
    }

    Vector<String> lines;
    for (; optind < argc; optind++) {
        if (read_all_path(argv[optind], lines)) {
            return 1;
        }
    }

    sort(lines);
    lines.for_each([&](auto& s) {
        puts(s.string());
    });

    return 0;
}
