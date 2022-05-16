#include <errno.h>
#include <ext/path.h>
#include <liim/function.h>
#include <liim/result.h>
#include <liim/string.h>
#include <stdlib.h>
#include <unistd.h>

#include "context.h"

namespace PortManager {
Result<Monostate, Monostate> Context::run_command(const String& command) {
    int status = system(command.string());
    if (status) {
        return Err(Monostate {});
    }
    return Ok(Monostate {});
}

Result<Monostate, Error> Context::with_working_directory(const Ext::Path& working_directory, Function<Result<Monostate, Error>()> body) {
    auto old_working_directory = String::wrap_malloced_chars(getcwd(nullptr, 0));
    if (chdir(working_directory.to_string().string())) {
        return Err(Ext::StringError(format("Failed to cd to `{}': {}", working_directory, strerror(errno))));
    }

    auto result = body();

    if (chdir(old_working_directory->string())) {
        return Err(Ext::StringError(format("Failed to cd to `{}': {}", *old_working_directory, strerror(errno))));
    }

    return result;
}
}
