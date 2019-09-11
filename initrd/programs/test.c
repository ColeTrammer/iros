#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {

    /* Test Reading Files */
    
    int fd = open("/a.txt", 0, 0);
    char *buffer = malloc(0x5000);
    if (buffer == NULL) {
        while (1);
    }
    read(fd, buffer, 0x5000);
    puts(buffer);
    close(fd);

    /* Test Fork Sys Call */

    // pid_t ret = fork();
    // if (ret == 0) {
    //     for (int i = 0; i < 1300; i++) {
    //         printf("%d: Child\n", i);
    //     }
    //     return EXIT_SUCCESS;
    // }

    // for (int i = 0; i <= 1100; i++) {
    //     printf("%d: Parent\n", i);
    // }

    return EXIT_SUCCESS;
}