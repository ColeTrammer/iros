#pragma once

#include <graphics/color.h>
#include <liim/vector.h>

class TTY {
public:
    struct Cell {
        Color fg { ColorValue::White };
        Color bg { ColorValue::Black };
        char ch { ' ' };
        bool bold { false };
        bool inverted { false };
        mutable bool dirty { true };
    };

    using Row = Vector<Cell>;

    int cursor_row() const { return m_cursor_row; }
    int cursor_col() const { return m_cursor_col; }

    void resize(int rows, int cols);
    void on_char(char c);

    const Vector<Row>& rows() const { return m_rows; }

private:
    void put_char(char c);

    int m_row_count { 0 };
    int m_col_count { 0 };
    int m_cursor_row { 0 };
    int m_cursor_col { 0 };
    Vector<Row> m_rows;
};
