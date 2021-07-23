#include <stdio.h>
#include <string.h>

#include "../builtin.h"

static int op_unalias(int argc, char **argv) {
    if (argc == 1) {
        fprintf(stderr, "Usage: %s [-a] name [...]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-a") == 0) {
        g_aliases.clear();
        return 0;
    }

    bool any_failed = false;
    for (int i = 1; i < argc; i++) {
        String s(argv[i]);

        if (!g_aliases.get(s)) {
            any_failed = true;
            continue;
        }
        g_aliases.remove(s);
    }

    return any_failed ? 1 : 0;
}
SH_REGISTER_BUILTIN(unalias, op_unalias);
