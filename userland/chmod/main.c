#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

void print_usage_and_exit(char **argv) {
    printf("Usage: %s <octal-number> <file-name>\n", argv[0]);
    exit(0);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        print_usage_and_exit(argv);
    }

    mode_t mode;
    if (sscanf(argv[1], "%o", &mode) != 1) {
        print_usage_and_exit(argv);
    }

    if (chmod(argv[2], mode) != 0) {
        perror("chmod");
        return 1;
    }

    return 0;
}