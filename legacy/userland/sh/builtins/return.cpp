#include <stdio.h>

#include "../builtin.h"
#include "../command.h"

static int op_return(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: return [n]\n");
        return 2;
    }

    int status = 0;
    if (argc == 2) {
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
