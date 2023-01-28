#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../builtin.h"

static int op_export(int argc, char **argv) {
    if (argc == 1) {
        printf("Usage: %s <key=value>...\n", argv[0]);
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        char *equals = strchr(argv[i], '=');
        if (equals == NULL) {
            fprintf(stderr, "Invalid environment string: %s\n", argv[i]);
            continue;
        }
        *equals = '\0';

        if (setenv(argv[i], equals + 1, 1)) {
            perror("shell");
            return 0;
        }
    }

    return 0;
}
SH_REGISTER_BUILTIN(export, op_export);
