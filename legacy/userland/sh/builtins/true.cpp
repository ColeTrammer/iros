#include "../builtin.h"

static int op_true(int, char **) {
    return 0;
}
SH_REGISTER_BUILTIN(true, op_true);
