#include <stdio.h>
#include <stdlib.h>

#include "../builtin.h"
#include "../command.h"

static int op_break(int argc, char **argv) {
    if (get_loop_depth_count() == 0) {
        fprintf(stderr, "Break is meaningless outside of for,while,until.\n");
        return 0;
    }

    if (argc > 2) {
        fprintf(stderr, "Usage: break [n]\n");
        return 2;
    }

    int break_count = 1;
    if (argc == 2) {
        break_count = atoi(argv[1]);
    }

    if (break_count == 0) {
        fprintf(stderr, "Invalid loop number.\n");
        return 1;
    }

    set_break_count(break_count);
    return 0;
}
SH_REGISTER_BUILTIN(break, op_break);
