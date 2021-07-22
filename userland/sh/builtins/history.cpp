#include <stdio.h>

#include "../builtin.h"
#include "../input.h"

static int op_history(char **argv) {
    if (argv[1]) {
        printf("Usage: %s\n", argv[0]);
        return 0;
    }

    ShRepl::the().history().print_history();
    return 0;
}
SH_REGISTER_BUILTIN(history, op_history);
