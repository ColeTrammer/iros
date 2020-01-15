#include <stdio.h>

int main(int argc, char **argv, char **envp) {
    (void) argc;
    (void) argv;

    for (size_t i = 0; envp[i] != NULL; i++) {
        puts(envp[i]);
    }

    return 0;
}