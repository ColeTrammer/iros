#include <stdio.h>
#include <stdlib.h>

#include "../builtin.h"

static int op_unset(char **argv) {
    if (!argv[1]) {
        printf("Usage: %s <key>\n", argv[0]);
        return 0;
    }

    for (size_t i = 1; argv[i] != NULL; i++) {
        if (unsetenv(argv[i])) {
            perror("shell");
            return 0;
        }
    }

    return 0;
}
SH_REGISTER_BUILTIN(unset, op_unset);
