#pragma once

#include <liim/forward.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace Ext {
class Path {
public:
    static Maybe<Path> resolve(const String& path);

    Path();
    ~Path();

    const String& basename() const { return m_components.last(); };
    String to_string() const;

    String join_component(const String& name) const;

private:
    Path(const String& path);

    Vector<String> m_components;
};
}
