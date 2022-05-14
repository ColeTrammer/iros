#include <ext/path.h>
#include <limits.h>
#include <unistd.h>

namespace Ext {
Option<Path> Path::resolve(const String& path) {
    if (path[0] == '/') {
        return { Path(path) };
    }

    char buffer[PATH_MAX];
    if (!realpath(path.string(), buffer)) {
        return {};
    }

    return { Path(buffer) };
}

Path Path::root() {
    return Path { "/" };
}

Path::Path(const String& path) {
    m_components = path.split('/');
}

Path::Path(Vector<String> components) : m_components(move(components)) {}

Path::Path() {}

Path::~Path() {}

void Path::set_to_parent() {
    if (!m_components.empty()) {
        m_components.remove_last();
    }
}

String Path::basename() const {
    if (m_components.empty()) {
        return "/";
    }
    return m_components.last();
}

Path Path::dirname() const {
    if (m_components.size() <= 1) {
        return root();
    }

    auto new_components = m_components;
    new_components.remove_last();
    return Path { move(new_components) };
}

String Path::to_string() const {
    return String::join(m_components, '/', JoinPrependDelimiter::Yes);
}

Path Path::join_component(const String& name) const {
    if (m_components.empty()) {
        return Path { String::format("/%s", name.string()) };
    }
    return Path { String::format("%s/%s", to_string().string(), name.string()) };
}
}
