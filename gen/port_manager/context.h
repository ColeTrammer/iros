#pragma once

#include <liim/forward.h>

#include "forward.h"

namespace PortManager {
class Context {
public:
    Result<Monostate, Monostate> run_command(const String& command);
};
}
