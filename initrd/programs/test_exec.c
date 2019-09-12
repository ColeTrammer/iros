#include <stddef.h>
#include <stdio.h>

int main(/* int argc, char **argv, char **envp */) {
    for (size_t i = 0; i <= 4; i++) {
        printf("Exec Test: %lu\n", i);
    }

    return 0;
}