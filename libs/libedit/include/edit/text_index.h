#pragma once

#include <liim/format.h>

namespace Edit {
class TextIndex {
public:
    TextIndex() {}
    TextIndex(int line_index, int index_into_line) : m_line_index(line_index), m_index_into_line(index_into_line) {}

    TextIndex offset(const TextIndex& other) const {
        return { line_index() + other.line_index(), index_into_line() + other.index_into_line() };
    }

    int line_index() const { return m_line_index; }
    int index_into_line() const { return m_index_into_line; }

    void set_line_index(int index) { m_line_index = index; }
    void set_index_into_line(int index) { m_index_into_line = index; }
    void set(int line_index, int index_into_line) {
        m_line_index = line_index;
        m_index_into_line = index_into_line;
    }

    bool operator==(const TextIndex& other) const = default;
    bool operator!=(const TextIndex& other) const = default;
    bool operator<(const TextIndex& other) const { return this->is_before(other); }
    bool operator<=(const TextIndex& other) const { return (*this == other) || this->is_before(other); }
    bool operator>(const TextIndex& other) const { return this->is_after(other); }
    bool operator>=(const TextIndex& other) const { return (*this == other) || this->is_after(other); }

    bool is_before(const TextIndex& other) const {
        if (this->line_index() < other.line_index()) {
            return true;
        }

        if (this->line_index() == other.line_index()) {
            return this->index_into_line() < other.index_into_line();
        }
        return false;
    }

    bool is_after(const TextIndex& other) const {
        if (this->line_index() > other.line_index()) {
            return true;
        }

        if (this->line_index() == other.line_index()) {
            return this->index_into_line() > other.index_into_line();
        }
        return false;
    }

private:
    int m_line_index { 0 };
    int m_index_into_line { 0 };
};
}

namespace LIIM::Format {
template<>
struct Formatter<Edit::TextIndex> : public Formatter<String> {
    void format(const Edit::TextIndex& index, FormatContext& context) {
        return format_to_context(context, "TextIndex <line_index={} index_into_line={}>", index.line_index(), index.index_into_line());
    }
};
}
