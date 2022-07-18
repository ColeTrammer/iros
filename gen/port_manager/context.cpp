#include <errno.h>
#include <ext/path.h>
#include <liim/container/hash_set.h>
#include <liim/function.h>
#include <liim/result.h>
#include <liim/string.h>
#include <liim/try.h>
#include <stdlib.h>
#include <unistd.h>

#include "context.h"
#include "port.h"
#include "process.h"
#include "step.h"

namespace PortManager {
Result<void, Error> Context::run_process(Process process) {
    return process.spawn_and_wait();
}

Result<void, Error> Context::with_working_directory(const Ext::Path& working_directory, Function<Result<void, Error>()> body) {
    auto old_working_directory = String::wrap_malloced_chars(getcwd(nullptr, 0));
    if (chdir(working_directory.to_string().string())) {
        return Err(make_string_error("Failed to cd to `{}': {}", working_directory, strerror(errno)));
    }

    auto result = body();

    if (chdir(old_working_directory->string())) {
        return Err(make_string_error("Failed to cd to `{}': {}", *old_working_directory, strerror(errno)));
    }

    return result;
}

Result<PortHandle, Error> Context::load_port(Ext::Path path) {
    auto port = TRY(Port::create(*this, move(path)));
    auto handle = port.handle();
    m_ports.try_emplace(clone(handle), move(port));
    TRY(m_ports.at(handle)->load_dependencies(*this));
    return handle;
}

Result<void, Error> Context::load_port_dependency(PortHandle handle, const PortHandle& parent) {
    auto path = config().port_json_for_port(handle);
    TRY(m_ports.insert_with_factory(handle, [&](auto* pointer) {
        out_log("Loading dependency of `{}': `{}'", parent, handle);
        return LIIM::create_at(pointer, piecewise_construct, forward_as_tuple<const PortHandle&>(handle),
                               forward_as_tuple<Context&, Ext::Path>(*this, move(path)));
    }));
    return m_ports.at(handle)->load_dependencies(*this);
}

Result<void, Error> Context::build_port(const PortHandle& handle, StringView build_step) {
    // Perform a topological sort on the dependency graph for the specified port, and then
    // build each port in that order.
    // The underlying algorithm is the simple BFS algorithm described here:
    // https://en.wikipedia.org/wiki/Topological_sorting#Depth-first_search

    auto topo_sorted_ports = NewVector<Port*> {};
    auto unvisited_ports = collect_hash_set(m_ports.keys());
    auto temp_visited_ports = LIIM::Container::HashSet<PortHandle> {};

    Function<Result<void, Error>(const PortHandle&)> visit = [&](const PortHandle& handle) -> Result<void, Error> {
        temp_visited_ports.insert(handle);

        auto& port = *m_ports.at(handle);
        for (auto& dependency : port.dependencies()) {
            if (!unvisited_ports.contains(dependency)) {
                continue;
            }
            if (temp_visited_ports.contains(dependency)) {
                return Err(make_string_error("Cyclic dependency detected between `{}' and `{}'", handle, dependency));
            }
            TRY(visit(dependency));
        }

        temp_visited_ports.erase(handle);
        unvisited_ports.erase(handle);
        topo_sorted_ports.push_back(&port);
        return {};
    };

    TRY(visit(handle));

    auto step_for_dependencies = build_step == "clean"sv ? "clean"sv : "install"sv;
    return Ext::stop_on_error(topo_sorted_ports, [&](auto* port) {
        if (port != topo_sorted_ports.back()) {
            return port->build(*this, step_for_dependencies);
        } else {
            return port->build(*this, build_step);
        }
    });
}
}
