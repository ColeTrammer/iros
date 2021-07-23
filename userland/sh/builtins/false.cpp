#include "../builtin.h"

static int op_false(int, char **) {
    return 1;
}
SH_REGISTER_BUILTIN(false, op_false);
