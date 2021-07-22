#include <assert.h>
#include <errno.h>
#include <ext/parse_mode.h>
#include <fcntl.h>
#include <liim/hash_map.h>
#include <liim/string.h>
#include <liim/vector.h>
#include <sh/sh_lexer.h>
#include <sh/sh_parser.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <time.h>
#include <tinput/file_input_source.h>
#include <unistd.h>

#ifndef USERLAND_NATIVE
#include <wordexp.h>
#else
#include "../../libs/libc/include/wordexp.h"
#endif /* USERLAND_NATIVE */

#include "builtin.h"
#include "command.h"
#include "input.h"
#include "job.h"
#include "sh_state.h"

namespace Sh {
BuiltInManager &BuiltInManager::the() {
    static BuiltInManager s_the;
    return s_the;
}

void BuiltInManager::register_builtin(String name, Function<int(char **)> entry) {
    auto builtin = BuiltInOperation { move(name), move(entry) };
    m_builtins.put(builtin.name(), move(builtin));
}

void BuiltInManager::unregister_builtin(const String &name) {
    m_builtins.remove(String(name));
}

BuiltInOperation *BuiltInManager::find(const String &name) {
    return m_builtins.get(name);
}
}
