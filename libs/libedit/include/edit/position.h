#pragma once

#include <liim/format.h>

namespace Edit {
struct Position {
    int row { 0 };
    int col { 0 };

    bool operator==(const Position& other) const = default;
    bool operator!=(const Position& other) const = default;
    bool operator<=(const Position& other) const { return *this == other || *this < other; }
    bool operator>=(const Position& other) const { return *this == other || *this > other; }

    bool operator<(const Position& other) const {
        if (this->row < other.row) {
            return true;
        }

        if (this->row == other.row) {
            return this->col < other.col;
        }
        return false;
    }

    bool operator>(const Position& other) const {
        if (this->row > other.row) {
            return true;
        }

        if (this->row == other.row) {
            return this->col > other.col;
        }
        return false;
    }
};
}

namespace LIIM::Format {
template<>
struct Formatter<Edit::Position> : public Formatter<String> {
    void format(const Edit::Position& p, FormatContext& context) {
        return Formatter<String>::format(::format("Position <row={} col={}>", p.row, p.col), context);
    }
};
}