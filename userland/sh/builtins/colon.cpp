#include "../builtin.h"

static int op_colon(char **) {
    return 0;
}
SH_REGISTER_BUILTIN( :, op_colon);
