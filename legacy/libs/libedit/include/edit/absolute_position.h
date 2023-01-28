#pragma once

#include <liim/format.h>

namespace Edit {
class AbsolutePosition {
public:
    AbsolutePosition() = default;
    AbsolutePosition(int line_index, int relative_row, int relative_col)
        : m_line_index(line_index), m_relative_row(relative_row), m_relative_col(relative_col) {}

    int line_index() const { return m_line_index; }
    int relative_row() const { return m_relative_row; }
    int relative_col() const { return m_relative_col; }

    void set_line_index(int l) { set(l, m_relative_row, m_relative_col); }
    void set_relative_row(int r) { set(m_line_index, r, m_relative_col); }
    void set_relative_col(int c) { set(m_line_index, m_relative_row, c); }
    void set(int l, int r, int c) {
        m_line_index = l;
        m_relative_row = r;
        m_relative_col = c;
    }

    bool operator==(const AbsolutePosition& other) const = default;
    bool operator!=(const AbsolutePosition& other) const = default;
    bool operator<=(const AbsolutePosition& other) const { return *this == other || *this < other; }
    bool operator>=(const AbsolutePosition& other) const { return *this == other || *this > other; }

    bool operator<(const AbsolutePosition& other) const {
        if (this->line_index() < other.line_index()) {
            return true;
        }
        if (this->line_index() > other.line_index()) {
            return false;
        }
        if (this->relative_row() < other.relative_row()) {
            return true;
        }
        if (this->relative_row() > other.relative_row()) {
            return false;
        }
        return this->relative_col() < other.relative_col();
    }

    bool operator>(const AbsolutePosition& other) const {
        if (this->line_index() < other.line_index()) {
            return false;
        }
        if (this->line_index() > other.line_index()) {
            return true;
        }
        if (this->relative_row() < other.relative_row()) {
            return false;
        }
        if (this->relative_row() > other.relative_row()) {
            return true;
        }
        return this->relative_col() > other.relative_col();
    }

private:
    int m_line_index { 0 };
    int m_relative_row { 0 };
    int m_relative_col { 0 };
};
}

namespace LIIM::Format {
template<>
struct Formatter<Edit::AbsolutePosition> : public BaseFormatter {
    void format(const Edit::AbsolutePosition& p, FormatContext& context) {
        return format_to_context(context, "AbsolutePosition <line_index={} row={} col={}>", p.line_index(), p.relative_row(),
                                 p.relative_col());
    }
};
}
