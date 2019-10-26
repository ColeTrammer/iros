#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <search> <file>\n", argv[0]);
        return 0;
    }

    FILE *file = fopen(argv[2], "r");
    if (!file) {
        perror("grep");
        return 1;
    }

    char *pattern = argv[1];
    size_t pattern_len = strlen(pattern);

    char *line = NULL;
    size_t line_max = 0;

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
            printf("%s%s%s%s", s, "\033[31m", pattern, "\033[0m");
            s = match + pattern_len;
            matched = true;
        }

        if (!matched) {
            continue;
        }

        printf("%s\n", s);
    }

    free(line);

    if (fclose(file)) {
        perror("grep");
        return 1;
    }

    return 0;
}