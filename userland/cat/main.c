#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static void do_cat(FILE *file) {
    char buf[BUFSIZ];
    size_t nread;
    while ((nread = fread(buf, sizeof(char), BUFSIZ, file)) != 0) {
        fwrite(buf, sizeof(char), nread, stdout);
    }

    if (ferror(file)) {
        perror("fread");
        exit(1);
    }
}

void cat(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        perror("cat");
        exit(1);
    }

    do_cat(file);

    if (fclose(file)) {
        perror("cat");
        exit(1);
    }
}

int main(int argc, char **argv) {
    if (argc == 1) {
        do_cat(stdin);
    } else {
        for (int i = 1; i < argc; i++) {
            cat(argv[i]);
        }
    }

    return 0;
}