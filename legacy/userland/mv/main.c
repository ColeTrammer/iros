#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <old-path> <new-path>\n", argv[0]);
        return 0;
    }

    int ret = rename(argv[1], argv[2]);
    if (ret == -1) {
        perror("mv");
        return 1;
    }

    return 0;
}
