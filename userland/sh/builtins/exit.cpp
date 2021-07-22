#include <stdio.h>
#include <stdlib.h>

#include "../builtin.h"

static int op_exit(char **args) {
    int status = 0;
    if (args[1] != NULL) {
        if (args[2] != NULL) {
            printf("Usage: %s [n]\n", args[0]);
            return 1;
        }
        status = atoi(args[1]);
    }

    exit(status);
}
SH_REGISTER_BUILTIN(exit, op_exit);
