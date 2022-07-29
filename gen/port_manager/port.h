#pragma once

#include <ext/forward.h>
#include <liim/container/hash_map.h>
#include <liim/result.h>
#include <liim/string.h>

#include "forward.h"

namespace PortManager {
class Port {
public:
    static Result<Port, Error> create(Context& context, Path json_path);
    Port(Port&&) = default;
    Port& operator=(Port&&) = default;
    ~Port();

    StringView name() const { return m_name.view(); }
    StringView version() const { return m_version.view(); }

    PathView definition_file() const { return m_definition_file; }
    PathView definition_directory() const { return m_definition_directory; }
    PathView base_directory() const { return m_base_directory; }
    PathView source_directory() const { return m_source_directory; }
    PathView build_directory() const { return m_build_directory; }

    const PortHandle& handle() const { return m_name; }
    Span<const PortHandle> dependencies() const { return m_dependencies.span(); }

    Result<void, Error> build(Context& context, StringView build_step);
    Result<Step&, BuildStepNotFound> lookup_step(StringView step_name);
    Result<void, Error> load_dependencies(Context& context);

private:
    explicit Port(String name, String version, Path definition_file, Path definition_directory, Path base_directory, Path source_directory,
                  Path build_directory, LIIM::Container::HashMap<StringView, UniquePtr<Step>> steps, NewVector<PortHandle> dependencies);

    String m_name;
    String m_version;
    Path m_definition_file;
    Path m_definition_directory;
    Path m_base_directory;
    Path m_source_directory;
    Path m_build_directory;
    LIIM::Container::HashMap<StringView, UniquePtr<Step>> m_steps;
    NewVector<PortHandle> m_dependencies;
    bool m_loaded_dependencies { false };
};
}
