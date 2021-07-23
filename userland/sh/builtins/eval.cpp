#include <sh/sh_lexer.h>
#include <sh/sh_parser.h>
#include <stdio.h>

#include "../builtin.h"
#include "../command.h"

static int op_eval(int argc, char **argv) {
    if (argc == 1) {
        return 0;
    }

    String string;
    for (int i = 1; i < argc; i++) {
        string += argv[i];
        if (argv[i + 1]) {
            string += " ";
        }
    }

    ShLexer lexer(string.string(), string.size());
    if (!lexer.lex()) {
        fprintf(stderr, "syntax error: unexpected end of input\n");
        return 1;
    }

    ShParser parser(lexer);
    if (!parser.parse()) {
        return 1;
    }

    ShValue program(parser.result());
    command_run(program.program());
    return get_last_exit_status();
}
SH_REGISTER_BUILTIN(eval, op_eval);
