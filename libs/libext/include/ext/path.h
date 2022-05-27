#pragma once

#include <ext/error.h>
#include <liim/format.h>
#include <liim/forward.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace Ext {
class Path {
public:
    static Result<Path, SystemError> resolve(const String& path);
    static Path root();

    Path();
    ~Path();

    String basename() const;

    Path dirname() const;

    String to_string() const;

    void set_to_parent();

    Path join_component(const String& name) const;
    const Vector<String>& components() const { return m_components; }

    bool exists() const;

private:
    explicit Path(const String& path);
    explicit Path(Vector<String> path);

    Vector<String> m_components;
};
}
