#include <libgen.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        return 2;
    }

    puts(basename(argv[1]));
    return 0;
}