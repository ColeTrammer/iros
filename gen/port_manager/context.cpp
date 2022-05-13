#include <liim/result.h>
#include <liim/string.h>
#include <stdlib.h>

#include "context.h"

namespace PortManager {
Result<Monostate, Monostate> Context::run_command(const String& command) {
    int status = system(command.string());
    if (status) {
        return Err(Monostate {});
    }
    return Ok(Monostate {});
}
}
