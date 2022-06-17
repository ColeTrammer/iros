#pragma once

#include <ext/forward.h>
#include <liim/forward.h>

#include "config.h"

namespace PortManager {
class Context {
public:
    explicit Context(Config config) : m_config(move(config)) {}

    Result<void, Error> run_process(Process process);

    Result<void, Error> with_working_directory(const Ext::Path& working_directory, Function<Result<void, Error>()> body);

    const Config& config() const { return m_config; }

private:
    Config m_config;
};
}
