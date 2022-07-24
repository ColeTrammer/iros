#pragma once

#include <liim/container/algorithm/equal.h>
#include <liim/container/algorithm/lexographic_compare.h>
#include <liim/container/path_view_iterator.h>
#include <liim/format/builtin_formatters.h>
#include <liim/string.h>
#include <liim/string_view.h>
#include <liim/try.h>

namespace LIIM::Container {
class PathView {
public:
    constexpr PathView() : m_data("/"sv) {}
    PathView(const String& string) : m_data(string.view()) {}
    constexpr PathView(StringView data) : m_data(data) {}

    constexpr bool empty() const { return m_data.empty(); }
    constexpr StringView data() const { return m_data; }

    constexpr PathViewIterator begin() const { return PathViewIterator(data(), 0, 0); }
    constexpr PathViewIterator end() const { return PathViewIterator(data(), m_data.size(), m_data.size()); }

    constexpr bool is_absolute() const { return m_data.starts_with("/"); }
    constexpr bool is_relative() const { return !is_absolute(); }

    constexpr Option<StringView> filename() const {
        if (empty() || m_data.ends_with("/"sv)) {
            return None {};
        }
        auto trailing_slash = m_data.last_index_of('/');
        if (!trailing_slash) {
            return data();
        }
        return m_data.substring(*trailing_slash + 1);
    }

    constexpr Option<StringView> extension() const {
        return filename().map(split_filename).and_then([](auto tuple) {
            return tuple_get<1>(tuple);
        });
    }

    constexpr Option<StringView> stem() const {
        return filename().map(split_filename).and_then([](auto tuple) {
            return tuple_get<0>(tuple);
        });
    }

    constexpr Option<PathView> parent_path() const {
        if (m_data.empty() || m_data == "/"sv || m_data == "/"sv) {
            return None {};
        }

        auto result = PathView(*this);
        result.strip_filename();
        if (result.empty()) {
            return None {};
        }
        return result;
    }

    constexpr friend bool operator==(PathView a, PathView b) { return equal(a, b); }
    constexpr friend std::strong_ordering operator<=>(PathView a, PathView b) { return lexographic_compare(a, b); }

private:
    constexpr static Tuple<Option<StringView>, Option<StringView>> split_filename(StringView filename) {
        auto last_dot_index = filename.last_index_of('.');
        if (!last_dot_index || *last_dot_index == 0) {
            return { filename, None {} };
        }
        return { filename.first(*last_dot_index), filename.substring(*last_dot_index + 1) };
    }

    constexpr void strip_filename() {
        while (m_data.ends_with("/")) {
            m_data = m_data.first(m_data.size() - 1);
        }
        while (!m_data.empty() && !m_data.ends_with("/")) {
            m_data = m_data.first(m_data.size() - 1);
        }
        while (m_data.size() > 2 && m_data.ends_with("/")) {
            m_data = m_data.first(m_data.size() - 1);
        }
    }

    StringView m_data;
};
}

namespace LIIM::Format {
template<>
struct Formatter<LIIM::Container::PathView> : public Formatter<StringView> {
    void format(LIIM::Container::PathView path, FormatContext& context) { return Formatter<StringView>::format(path.data(), context); }
};
}

constexpr LIIM::Container::PathView operator""_pv(const char* data, size_t size) {
    return LIIM::Container::PathView(StringView(data, size));
}

using LIIM::Container::PathView;
