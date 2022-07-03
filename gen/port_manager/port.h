#pragma once

#include <ext/forward.h>
#include <liim/container/hash_map.h>
#include <liim/result.h>
#include <liim/string.h>

#include "forward.h"

namespace PortManager {
class Port {
public:
    static Result<Port, Error> create(Context& context, Ext::Path json_path);
    Port(Port&&) = default;
    Port& operator=(Port&&) = default;
    ~Port();

    const String& name() const { return m_name; }
    const String& version() const { return m_version; }

    const Ext::Path& definition_file() const { return m_definition_file; }
    const Ext::Path& definition_directory() const { return m_definition_directory; }
    const Ext::Path& base_directory() const { return m_base_directory; }
    const Ext::Path& source_directory() const { return m_source_directory; }
    const Ext::Path& build_directory() const { return m_build_directory; }

    const PortHandle& handle() const { return m_name; }
    const Span<const PortHandle> dependencies() const { return m_dependencies.span(); }

    Result<void, Error> build(Context& context, StringView build_step);
    Result<Step&, BuildStepNotFound> lookup_step(StringView step_name);
    Result<void, Error> load_dependencies(Context& context);

private:
    explicit Port(String name, String version, Ext::Path definition_file, Ext::Path definition_directory, Ext::Path base_directory,
                  Ext::Path source_directory, Ext::Path build_directory, LIIM::Container::HashMap<StringView, UniquePtr<Step>> steps,
                  NewVector<PortHandle> dependencies);

    String m_name;
    String m_version;
    Ext::Path m_definition_file;
    Ext::Path m_definition_directory;
    Ext::Path m_base_directory;
    Ext::Path m_source_directory;
    Ext::Path m_build_directory;
    LIIM::Container::HashMap<StringView, UniquePtr<Step>> m_steps;
    NewVector<PortHandle> m_dependencies;
    bool m_loaded_dependencies { false };
};
}
