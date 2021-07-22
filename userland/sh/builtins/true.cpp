#include "../builtin.h"

static int op_true(char **) {
    return 0;
}
SH_REGISTER_BUILTIN(true, op_true);
