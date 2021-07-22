#include <stdio.h>
#include <stdlib.h>

#include "../builtin.h"
#include "../command.h"

static int op_break(char **argv) {
    int break_count = 1;
    if (argv[1] != NULL) {
        break_count = atoi(argv[1]);
    }

    if (get_loop_depth_count() == 0) {
        fprintf(stderr, "Break is meaningless outside of for,while,until.\n");
        return 0;
    }

    if (break_count == 0) {
        fprintf(stderr, "Invalid loop number.\n");
        return 1;
    }

    set_break_count(break_count);
    return 0;
}
SH_REGISTER_BUILTIN(break, op_break);
