#include <ext/path.h>
#include <ext/system.h>
#include <liim/try.h>
#include <limits.h>
#include <unistd.h>

namespace Ext {
Result<Path, SystemError> Path::resolve(const String& path) {
    if (path[0] == '/') {
        return Ok(Path(path));
    }

    auto resolved = TRY(realpath(path.string()));
    return Ok(Path(move(resolved)));
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

bool Path::exists() const {
    // FIXME: should this function return errors?
    return access(to_string().string(), F_OK) == F_OK;
}
}
