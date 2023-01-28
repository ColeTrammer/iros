#include <stdio.h>

#include "../builtin.h"
#include "../command.h"

static int op_shift(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: shift [n]\n");
        return 2;
    }

    int amount = 1;
    if (argc == 2) {
        amount = atoi(argv[1]);
    }

    if (amount == 1 && command_position_params_size() == 0) {
        return 0;
    }

    if (amount < 0 || amount > (int) command_position_params_size()) {
        fprintf(stderr, "Invalid shift amount\n");
        return 1;
    }

    command_shift_position_params_left(amount);
    return 0;
}
SH_REGISTER_BUILTIN(shift, op_shift);
