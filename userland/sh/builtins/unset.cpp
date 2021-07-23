#include <stdio.h>
#include <stdlib.h>

#include "../builtin.h"

static int op_unset(int argc, char **argv) {
    if (argc == 1) {
        printf("Usage: %s <key>\n", argv[0]);
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (unsetenv(argv[i])) {
            perror("shell");
            return 0;
        }
    }

    return 0;
}
SH_REGISTER_BUILTIN(unset, op_unset);
