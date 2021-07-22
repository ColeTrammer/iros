#include <stdio.h>

#include "../builtin.h"
#include "../command.h"

static int op_return(char **argv) {
    int status = 0;
    if (argv[1] != NULL) {
        if (argv[2] != NULL) {
            fprintf(stderr, "Usage: %s [status]\n", argv[0]);
            return 1;
        }

        status = atoi(argv[1]);
    }

    if (get_exec_depth_count() == 0) {
        fprintf(stderr, "Cannot return when not in function or . script\n");
        return 1;
    }

    set_should_return();
    return status;
}
SH_REGISTER_BUILTIN(return, op_return);
