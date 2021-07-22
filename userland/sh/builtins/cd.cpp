#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../builtin.h"
#include "../input.h"

static int op_cd(char **args) {
    if (args[2]) {
        printf("Usage: %s <dir>\n", args[0]);
        return 0;
    }

    char *dir = NULL;
    if (!args[1]) {
        dir = getenv("HOME");
    } else {
        dir = args[1];
    }

    int ret = chdir(dir);
    if (ret != 0) {
        perror("Shell");
    }

    __refreshcwd();
    return 0;
}
SH_REGISTER_BUILTIN(cd, op_cd);
