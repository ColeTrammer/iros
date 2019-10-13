#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    (void) argc;
    (void) argv;

    /* Test execve */

    pid_t f = fork();
    if (f == 0) {
        execvp("ls", argv);

        perror("Test");

        /* Should Not Execute */
        return 1;
    } else if (f < 0) {
        perror("Test");
        return 1;
    } else {
        int status;

        do {
             waitpid(f, &status, WUNTRACED);
        } while (!WIFEXITED(status));
    }

    /* Test args */
    
    // printf("Argc: %d\n", argc);
    // for (size_t i = 0; argv[i] != NULL; i++) {
    //     printf("Argv[%ld]: %s\n", i, argv[i]);
    // }

    // printf("OS: %s\n", getenv("OS"));

    /* Test Writing To stdio */

    // const char *test_str = "Testing STDIO";
    // puts(test_str);

    /* Test Reading Files Using stdio.h */

    // FILE *a = fopen("/a.txt", "r");
    // char *buffer = malloc(0x5000);
    // fread(buffer, 0x5000, 1, a);
    // puts(buffer);
    // fclose(a);

    /* Test Reading Files Using Sys Calls */
    
    // int fd = open("/a.txt", 0, 0);
    // char *buffer = malloc(0x5000);
    // read(fd, buffer, 0x5000);
    // puts(buffer);
    // close(fd);

    /* Test Fork Sys Call */

    pid_t ret = fork();
    if (ret == 0) {
        for (int i = 0; i <= 2; i++) {
            printf("%d: Child\n", i);
        }
        return 0;
    }

    for (int i = 0; i <= 3; i++) {
        printf("%d: Parent\n", i);
    }

    return 0;
}