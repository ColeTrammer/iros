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

    if (argc > 2) {
        printf("Usage: %s [file-name]\n", argv[0]);
        return 0;
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("cat");
        return 1;
    }

    if (fseek(file, 0, SEEK_END) < 0) {
        perror("cat");
        return 1;
    }

    off_t size = ftell(file);
    if (size < 0) {
        perror("cat");
        return 1;
    }    

    if (fseek(file, 0, SEEK_SET) < 0) {
        perror("cat");
        return 1;
    }

    char *buffer = malloc(size + 1);
    int read = fread(buffer, 1, size, file);
    if (read == 0) {
        perror("cat");
        return 1;
    }
    buffer[size] = '\0';
    printf("%s", buffer);
    fclose(file);

    return 0;
}