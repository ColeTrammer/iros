#include <stdio.h>

#include "../builtin.h"

static String sh_escape(const String &string) {
    // FIXME: should escape the literal `'` with `'\''`
    return String::format("'%s'", string.string());
}

static int op_alias(int argc, char **argv) {
    if (argc == 1) {
        g_aliases.for_each_key([&](const String &name) {
            printf("alias %s=%s\n", name.string(), sh_escape(*g_aliases.get(name)).string());
        });
        return 0;
    }

    bool any_failed = false;
    for (int i = 1; i < argc; i++) {
        String arg(argv[i]);
        auto equal_index = arg.index_of('=');
        if (!equal_index.has_value()) {
            auto *alias_name = g_aliases.get(arg);
            if (!alias_name) {
                any_failed = true;
                continue;
            }

            printf("alias %s=%s\n", arg.string(), sh_escape(*alias_name).string());
            continue;
        }

        arg[equal_index.value()] = '\0';
        String name(arg.string());
        String alias(arg.string() + equal_index.value() + 1);
        g_aliases.put(name, alias);
    }

    return any_failed ? 1 : 0;
}
SH_REGISTER_BUILTIN(alias, op_alias);
