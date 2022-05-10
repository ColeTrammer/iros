#pragma once

#include <ext/forward.h>
#include <liim/result.h>
#include <liim/string.h>
#include <liim/vector.h>

#include "forward.h"

namespace PortManager {
class Port {
public:
    static Result<Port, String> try_create(const Ext::Path& path);
    Port(const Port&) = delete;
    Port(Port&&) = default;
    ~Port();

    const String& name() const { return m_name; }
    const String& version() const { return m_version; }

    Result<Monostate, String> build();

private:
    Port(String name, String version, Vector<UniquePtr<Step>> steps);

    String m_name;
    String m_version;
    Vector<UniquePtr<Step>> m_steps;
};
}
