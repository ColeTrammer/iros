#include <errno.h>
#include <liim/vector.h>
#include <stdio.h>

#ifndef USERLAND_NATIVE
#include <wordexp.h>
#else
#include "../../libs/libc/include/wordexp.h"
#endif /* USERLAND_NATIVE */

#include "../builtin.h"

static int op_read(char **argv) {
    Vector<char> input;

    bool prev_was_backslash = false;
    for (;;) {
        errno = 0;
        int ret = fgetc(stdin);
        if (ret == EOF) {
            if (errno == EINTR) {
                clearerr(stdin);
                continue;
            }
            break;
        }

        char c = (char) ret;
        if (c == '\\') {
            prev_was_backslash = true;
            input.add(c);
            continue;
        }

        if (c == '\n') {
            if (!prev_was_backslash) {
                break;
            }

            input.remove_last();
        } else {
            input.add(c);
        }
        prev_was_backslash = false;
    }

    input.add('\0');

    const char *ifs = getenv("IFS");
    const char *split_on = ifs ? ifs : " \t\n";

    wordexp_t exp;
    exp.we_offs = 0;
    exp.we_wordc = 0;
    exp.we_wordv = nullptr;
    int ret = we_split(input.vector(), split_on, &exp, 0);
    if (ret != 0) {
        return 1;
    }

    for (size_t i = 1; argv[i] != nullptr; i++) {
        if (exp.we_wordc <= i - 1) {
            break;
        }

        char *name = argv[i];
        setenv(name, exp.we_wordv[i - 1], 1);
    }

    wordfree(&exp);
    return 0;
}
SH_REGISTER_BUILTIN(read, op_read);
