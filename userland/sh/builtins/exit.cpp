#include <stdio.h>
#include <stdlib.h>

#include "../builtin.h"

static int op_exit(int argc, char **args) {
    if (argc > 2) {
        fprintf(stderr, "Usage: exit [n]\n");
        return 2;
    }

    int status = 0;
    if (argc == 2) {
        status = atoi(args[1]);
    }

    exit(status);
}
SH_REGISTER_BUILTIN(exit, op_exit);
