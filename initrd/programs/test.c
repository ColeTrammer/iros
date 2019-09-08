#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {

    pid_t ret = fork();
    if (ret == 0) {
        for (int i = 0; i < 1300; i++) {
            printf("%d: Child\n", i);
        }
        return EXIT_SUCCESS;
    }

    for (int i = 0; i <= 1100; i++) {
        printf("%d: Parent\n", i);
    }

    return EXIT_SUCCESS;
}