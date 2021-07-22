#include "../builtin.h"
#include "../command.h"
#include "../sh_state.h"

static int op_set(char **argv) {
    if (argv[1] == NULL) {
        // FIXME: should print out every shell variable.
        return 0;
    }

    int i = 1;
    for (; argv[i]; i++) {
        if (argv[i][0] == '-' || argv[i][0] == '+') {
            bool worked = false;
            if (argv[i][1] != 'o') {
                worked = ShState::the().process_arg(argv[i]);
            } else {
                bool to_set = argv[i][0] == '-';
                i++;

                if (argv[i]) {
                    ShState::the().process_option(argv[i], to_set);
                    worked = true;
                } else {
                    if (to_set) {
                        ShState::the().dump_for_reinput();
                    } else {
                        ShState::the().dump();
                    }
                    worked = false;
                }
            }

            if (worked) {
                continue;
            }
        }

        break;
    }

    for (; argv[i]; i++) {
        command_add_position_param(argv[i]);
    }

    return 0;
}
SH_REGISTER_BUILTIN(set, op_set);
