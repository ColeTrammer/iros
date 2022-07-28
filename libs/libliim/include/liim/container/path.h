#pragma once

#include <liim/container/path_view.h>
#include <liim/format.h>
#include <liim/string.h>

namespace LIIM::Container {
class Path {
public:
    static Path create(PathView view) { return Path(String(view.data())); }

    Path() {}
    Path(const Path&) = delete;
    Path(Path&&) = default;

    explicit Path(String&& data) : m_data(move(data)) {}

    Path& operator=(const Path&) = delete;
    Path& operator=(Path&&) = default;

    Path clone() const { return Path::create(view()); }

    bool empty() const { return view().empty(); }

    operator PathView() const { return view(); }
    PathView view() const { return PathView(m_data); }

    PathViewIterator begin() const { return view().begin(); }
    PathViewIterator end() const { return view().end(); }

    bool is_absolute() const { return view().is_absolute(); }
    bool is_relative() const { return view().is_relative(); }

    Option<StringView> filename() const { return view().filename(); }
    Option<StringView> extension() const { return view().extension(); }
    Option<StringView> stem() const { return view().stem(); }
    Option<PathView> parent_path() const { return view().parent_path(); }

    bool starts_with(PathView prefix) const { return view().starts_with(prefix); }
    bool ends_with(PathView suffix) const { return view().ends_with(suffix); }

    bool filename_ends_with(StringView suffix) const { return view().filename_ends_with(suffix); }

    friend bool operator==(const Path& a, const Path& b) { return a.view() == b.view(); }
    friend std::strong_ordering operator<=>(const Path& a, const Path& b) { return a.view() <=> b.view(); }

    void clear() { m_data.clear(); }

    template<Format::Formattable... Args>
    requires(sizeof...(Args) > 0) Path join(Format::FormatString<Args...> format_string, const Args&... args)
    const { return view().join(format_string, args...); }
    Path join(PathView path) const { return view().join(path); }

    template<Format::Formattable... Args>
    requires(sizeof...(Args) > 0) Path& append(Format::FormatString<Args...> format_string, const Args&... args);
    Path& append(PathView path);

    Path& replace_with_parent_path();

private:
    String m_data;
};

inline Path& Path::append(PathView path) {
    if (path.is_absolute()) {
        m_data = String(path.data());
    } else {
        if (m_data.ends_with("/")) {
            m_data += String(path.data());
        } else {
            m_data += "/"s;
            m_data += String(path.data());
        }
    }
    return *this;
}

template<Format::Formattable... Args>
requires(sizeof...(Args) > 0) Path& Path::append(Format::FormatString<Args...> format_string, const Args&... args) {
    auto to_append = vformat(format_string.data(), make_format_args(args...));
    return this->append(to_append);
}

inline Path& Path::replace_with_parent_path() {
    if (auto new_path = this->parent_path()) {
        m_data = String(new_path->data());
    }
    return *this;
}

inline Path PathView::join(PathView path) const {
    auto result = Path::create(*this);
    result.append(path);
    return result;
}

template<Format::Formattable... Args>
requires(sizeof...(Args) > 0) inline Path PathView::join(Format::FormatString<Args...> format_string, const Args&... args) const {
    auto result = Path::create(*this);
    result.append(format_string, args...);
    return result;
}
}

namespace LIIM::Format {
template<>
struct Formatter<LIIM::Container::Path> : public Formatter<PathView> {
    void format(const LIIM::Container::Path& path, FormatContext& context) { return Formatter<PathView>::format(path.view(), context); }
};
}

inline LIIM::Container::Path operator""_p(const char* data, size_t size) {
    return LIIM::Container::Path(String(data, size));
}

using LIIM::Container::Path;
