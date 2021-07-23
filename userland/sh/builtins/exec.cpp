#include <errno.h>
#include <unistd.h>

#include "../builtin.h"

static int op_exec(int argc, char **argv) {
    if (argc == 1) {
        return 0;
    }

    execvp(argv[1], argv + 1);
    if (errno == ENOENT) {
        return 127;
    }
    return 126;
}
SH_REGISTER_BUILTIN(exec, op_exec);
