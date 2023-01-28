#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 2;
    }

    char buf[PATH_MAX];
    if (!realpath(argv[1], buf)) {
        perror("realpath");
        return 1;
    }

    puts(buf);

    return 0;
}
