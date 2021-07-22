#include <stdio.h>
#include <stdlib.h>

#include "../builtin.h"
#include "../command.h"

static int op_continue(char **argv) {
    int continue_count = 1;
    if (argv[1] != NULL) {
        continue_count = atoi(argv[1]);
    }

    if (get_loop_depth_count() == 0) {
        fprintf(stderr, "Continue is meaningless outside of for,while,until.\n");
        return 0;
    }

    if (continue_count == 0) {
        fprintf(stderr, "Invalid loop number.\n");
        return 1;
    }

    set_continue_count(continue_count);
    return 0;
}
SH_REGISTER_BUILTIN(continue, op_continue);
