#include <stdio.h>
#include <string.h>

#include "../builtin.h"

static int op_echo(int argc, char **args) {
    if (argc == 1) {
        printf("%c", '\n');
        return 0;
    }

    int i = 1;
    bool print_new_line = true;
    if (strcmp(args[i], "-n") == 0) {
        print_new_line = false;
        i++;
    }

    for (; i < argc; i++) {
        printf("%s", args[i]);
        if (args[i + 1] != NULL) {
            printf("%c", ' ');
        } else {
            break;
        }
    }

    if (print_new_line) {
        printf("%c", '\n');
    }

    if (fflush(stdout)) {
        perror("echo");
        return 1;
    }
    return 0;
}
SH_REGISTER_BUILTIN(echo, op_echo);
