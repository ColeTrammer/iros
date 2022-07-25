#pragma once

#include <liim/container/path_view.h>
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

    bool starts_with(PathView prefix) const { return view().starts_with(prefix); }
    bool ends_with(PathView suffix) const { return view().ends_with(suffix); }

    bool filename_ends_with(StringView suffix) const { return view().filename_ends_with(suffix); }

    friend bool operator==(const Path& a, const Path& b) { return a.view() == b.view(); }
    friend std::strong_ordering operator<=>(const Path& a, const Path& b) { return a.view() <=> b.view(); }

private:
    String m_data;
};
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
