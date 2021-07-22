#include "../builtin.h"

static int op_false(char **) {
    return 1;
}
SH_REGISTER_BUILTIN(false, op_false);
