#pragma once

#include <liim/container/concepts.h>
#include <liim/string_view.h>
#include <liim/utilities.h>

namespace LIIM::Container {
class PathViewIterator {
public:
    using ValueType = StringView;

    constexpr ValueType operator*() const { return m_data.substring(m_token_start, m_token_end - m_token_start); }

    constexpr PathViewIterator& operator++() {
        if (m_token_start == 0 && m_token_end == 0) {
            // The begin iterator was just constructed, find the correct first token.
            if (m_data.starts_with("/") || m_data.starts_with("./")) {
                m_token_end = 1;
                return *this;
            }
            while (m_token_end < m_data.size() && m_data[m_token_end] != '/') {
                m_token_end++;
            }
            return *this;
        }

        while (m_token_end < m_data.size() && m_data[m_token_end] == '/') {
            m_token_end++;
        }
        if (m_token_end == m_data.size()) {
            // The last token has already been consumed
            m_token_start = m_token_end;
            return *this;
        }

        m_token_start = m_token_end++;
        while (m_token_end < m_data.size() && m_data[m_token_end] != '/') {
            m_token_end++;
        }
        if (m_token_start != 0 && **this == ".") {
            return ++*this;
        }
        return *this;
    }

    constexpr PathViewIterator& operator--() {
        assert(m_token_start > 0);
        if (m_token_start != m_token_end) {
            m_token_end = m_token_start - 1;
        }
        while (m_token_end > 0 && m_data[m_token_end - 1] == '/') {
            m_token_end--;
        }

        if (m_token_end == 0) {
            // The first component must be a slash, and should be returned.
            m_token_start = 0;
            m_token_end = 1;
            return *this;
        }

        m_token_start = m_token_end - 1;
        while (m_token_start > 0 && m_data[m_token_start - 1] != '/') {
            m_token_start--;
        }
        if (m_token_start != 0 && **this == ".") {
            return --*this;
        }
        return *this;
    }

    constexpr PathViewIterator operator++(int) {
        auto result = PathViewIterator(*this);
        ++*this;
        return result;
    }

    constexpr PathViewIterator operator--(int) {
        auto result = PathViewIterator(*this);
        --*this;
        return result;
    }

    constexpr bool operator==(const PathViewIterator& other) const { return this->m_token_start == other.m_token_start; }
    constexpr std::strong_ordering operator<=>(const PathViewIterator& other) const { return this->m_token_start <=> other.m_token_start; }

private:
    friend class PathView;

    constexpr PathViewIterator(StringView data, size_t token_start, size_t token_end)
        : m_data(data), m_token_start(token_start), m_token_end(token_end) {
        ++*this;
    }

    StringView m_data;
    size_t m_token_start { 0 };
    size_t m_token_end { 0 };
};
}
