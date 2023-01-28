#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../builtin.h"
#include "../input.h"

static int op_cd(int argc, char **argv) {
    if (argc > 2) {
        printf("Usage: %s <dir>\n", argv[0]);
        return 2;
    }

    char *dir = NULL;
    if (!argv[1]) {
        dir = getenv("HOME");
    } else {
        dir = argv[1];
    }

    int ret = chdir(dir);
    if (ret != 0) {
        perror("Shell");
    }

    __refreshcwd();
    return 0;
}
SH_REGISTER_BUILTIN(cd, op_cd);
