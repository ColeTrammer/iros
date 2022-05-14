#pragma once

#include <ext/forward.h>
#include <liim/forward.h>

#include "config.h"

namespace PortManager {
class Context {
public:
    Context(Config config) : m_config(move(config)) {}

    Result<Monostate, Monostate> run_command(const String& command);

    Result<Monostate, Error> with_working_directory(const Ext::Path& working_directory, Function<Result<Monostate, Error>()> body);

    const Config& config() const { return m_config; }

private:
    Config m_config;
};
}
