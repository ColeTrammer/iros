#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int main(/* int argc, char **argv, char **envp */) {
    size_t n = 0;
    char *line = NULL;
    ssize_t line_len;

    FILE *f = fopen("a.txt", "r");
    while ((line_len = getline(&line, &n, f)) != -1) {
        while (line[line_len - 1] == '\n' || line[line_len - 1] == '\r') {
            line[--line_len] = '\0';
        }
        printf("%s\n", line);
    }

    return 0;
}