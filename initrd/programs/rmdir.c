#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <dir-name>\n", argv[0]);
        return 0;
    }

    if (rmdir(argv[1]) != 0) {
        perror("rmdir");
        return 1;
    }

    return 0;
}