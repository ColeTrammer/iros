#include <ext/path.h>
#include <limits.h>
#include <unistd.h>

namespace Ext {
Maybe<Path> Path::resolve(const String& path) {
    if (path[0] == '/') {
        return { Path(path) };
    }

    char buffer[PATH_MAX];
    if (!realpath(path.string(), buffer)) {
        return {};
    }

    return { Path(buffer) };
}

Path::Path(const String& path) {
    m_components = path.split('/');
}

Path::Path() {
    m_components = Vector<String>::create_from_single_element("");
}

Path::~Path() {}

String Path::to_string() const {
    return String::join(m_components, '/');
}

String Path::join_component(const String& name) const {
    return String::format("%s/%s", to_string().string(), name.string());
}
}
