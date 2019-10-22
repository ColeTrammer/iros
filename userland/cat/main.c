#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int main(int argc, char **argv) {
    if (argc == 1) {
        int c;
        while ((c = getchar()) != EOF) {
            printf("%c", c);
        }

        return 0;
    }

    char *line = NULL;
    size_t line_max = 0;

    for (int i = 1; i < argc; i++) {
        FILE *f = fopen(argv[i], "r");
        if (f == NULL) {
            perror("cat");
            return 1;
        }

        while (getline(&line, &line_max, f) != -1) {
            printf("%s", line);
        }
    }

    return 0;
}