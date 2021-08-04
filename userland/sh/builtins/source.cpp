#include <repl/file_input_source.h>
#include <stdio.h>

#include "../builtin.h"
#include "../command.h"
#include "../input.h"

int op_source(int argc, char **argv) {
    if (argc == 1) {
        fprintf(stderr, "Usage: %s <filename> [args]\n", argv[0]);
        return 2;
    }

    auto path = String(argv[1]);
    auto input_source = Repl ::FileInputSource::create_from_path(ShRepl::the(), path);
    if (!input_source) {
        fprintf(stderr, "%s: Failed to open file `%s'\n", argv[0], path.string());
        return 1;
    }

    command_push_position_params(PositionArgs(argv + 2, argc - 2));

    inc_exec_depth_count();
    ShRepl::the().enter(*input_source);
    dec_exec_depth_count();
    return get_last_exit_status();
}
SH_REGISTER_BUILTIN(source, op_source);
SH_REGISTER_BUILTIN(., op_source);
