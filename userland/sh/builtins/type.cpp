#include <stdio.h>

#include "../builtin.h"
#include "../command.h"

static int op_type(int, char **argv) {
    while (*++argv) {
        auto word = String(*argv);

        auto *alias = g_aliases.get(word);
        if (alias) {
            printf("%s is aliased to `%s'\n", word.string(), alias->string());
            continue;
        }

        auto *function = g_functions.get(word);
        if (function) {
            printf("%s is a shell function\n", word.string());
            puts(function->for_posterity->string());
            continue;
        }

        auto *builtin = Sh::BuiltInManager::the().find(word);
        if (builtin) {
            printf("%s is a shell builtin\n", word.string());
            continue;
        }

        printf("%s is %s\n", word.string(), word.string());
    }
    return 0;
}
SH_REGISTER_BUILTIN(type, op_type);
