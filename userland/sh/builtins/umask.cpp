#include <ext/parse_mode.h>
#include <stdio.h>
#include <sys/stat.h>

#include "../builtin.h"

static int op_umask(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s <mode>\n", *argv);
        return 2;
    }

    if (argc == 1) {
        mode_t mask = umask(0);
        umask(mask);

        printf("%#.4o\n", mask);
        return 0;
    }

    auto fancy_mode = Ext::parse_mode(argv[1]);
    if (!fancy_mode.has_value()) {
        fprintf(stderr, "%s: failed to parse mode: `%s'\n", *argv, argv[1]);
        return 1;
    }

    if (fancy_mode.value().impl().is<mode_t>()) {
        umask(fancy_mode.value().resolve(0) & 0777);
        return 0;
    }

    mode_t mode = ~umask(0);
    mode = fancy_mode.value().resolve(mode, 0);
    umask(~mode);
    return 0;
}
SH_REGISTER_BUILTIN(umask, op_umask);
