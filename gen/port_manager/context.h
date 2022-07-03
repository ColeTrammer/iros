#pragma once

#include <ext/forward.h>
#include <liim/container/hash_map.h>
#include <liim/forward.h>

#include "config.h"
#include "port.h"

namespace PortManager {
class Context {
public:
    explicit Context(Config config) : m_config(move(config)) {}

    Result<void, Error> run_process(Process process);

    Result<void, Error> with_working_directory(const Ext::Path& working_directory, Function<Result<void, Error>()> body);

    Result<PortHandle, Error> load_port(Ext::Path path);
    Result<void, Error> load_port_dependency(PortHandle handle, const PortHandle& parent);
    Result<void, Error> build_port(const PortHandle& handle, StringView build_step);

    const Config& config() const { return m_config; }

private:
    Config m_config;
    LIIM::Container::HashMap<PortHandle, Port> m_ports;
};
}
