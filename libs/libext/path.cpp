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

String Path::dirname(SlashTerminated slash_terminated) const {
    if (m_components.size() <= 1) {
        return "/";
    }

    auto string = String {};
    for (int i = 0; i < m_components.size() - 1; i++) {
        string += "/";
        string += m_components[i];
    }
    if (slash_terminated == SlashTerminated::Yes) {
        string += "/";
    }
    return string;
}

String Path::to_string() const {
    return String::join(m_components, '/', JoinPrependDelimiter::Yes);
}

String Path::join_component(const String& name) const {
    if (m_components.empty()) {
        return String::format("/%s", name.string());
    }
    return String::format("%s/%s", to_string().string(), name.string());
}
}
