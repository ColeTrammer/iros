#pragma once

#include <liim/format.h>

namespace Edit {
class AbsolutePosition {
public:
    AbsolutePosition() = default;
    AbsolutePosition(int row, int col) : m_row(row), m_col(col) {}

    int row() const { return m_row; }
    int col() const { return m_col; }

    void set_row(int r) { set(r, m_col); }
    void set_col(int c) { set(m_row, c); }
    void set(int r, int c) {
        m_row = r;
        m_col = c;
    }

    bool operator==(const AbsolutePosition& other) const = default;
    bool operator!=(const AbsolutePosition& other) const = default;
    bool operator<=(const AbsolutePosition& other) const { return *this == other || *this < other; }
    bool operator>=(const AbsolutePosition& other) const { return *this == other || *this > other; }

    bool operator<(const AbsolutePosition& other) const {
        if (this->row() < other.row()) {
            return true;
        }

        if (this->row() == other.row()) {
            return this->col() < other.col();
        }
        return false;
    }

    bool operator>(const AbsolutePosition& other) const {
        if (this->row() > other.row()) {
            return true;
        }

        if (this->row() == other.row()) {
            return this->col() > other.col();
        }
        return false;
    }

private:
    int m_row { 0 };
    int m_col { 0 };
};
}

namespace LIIM::Format {
template<>
struct Formatter<Edit::AbsolutePosition> : public Formatter<String> {
    void format(const Edit::AbsolutePosition& p, FormatContext& context) {
        return Formatter<String>::format(::format("AbsolutePosition <row={} col={}>", p.row(), p.col()), context);
    }
};
}
