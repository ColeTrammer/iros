#include "tty.h"

void TTY::resize(int rows, int cols) {
    m_row_count = rows;
    m_col_count = cols;

    m_rows.resize(rows);
    for (auto& row : m_rows) {
        row.resize(cols);
    }

    for (auto& row : m_rows) {
        for (auto& cell : row) {
            cell.dirty = true;
        }
    }

    m_cursor_row = min(m_cursor_row, rows - 1);
    m_cursor_col = min(m_cursor_col, cols - 1);
}

void TTY::put_char(char c) {
    auto& cell = m_rows[m_cursor_row][m_cursor_col];
    cell.ch = c;
    cell.dirty = true;

    m_cursor_col++;
    if (m_cursor_col >= m_col_count) {
        m_cursor_col = 0;
        m_cursor_row++;
    }
}

void TTY::on_char(char c) {
    switch (c) {
        case '\0':
            return;
        case '\r':
            m_cursor_col = 0;
            break;
        case '\n':
            m_cursor_row++;
            break;
        // Ascii BS (NOTE: not the backspace key)
        case 8:
            if (m_cursor_col > 0) {
                m_cursor_col--;
            }
            break;
        // Ascii DEL (NOTE: not the delete key)
        case 127:
            if (m_cursor_col > 0) {
                m_cursor_col--;
                put_char(' ');
                m_cursor_col--;
            }
            break;
        case '\a':
            // Ignore alarm character for now
            break;
        default:
            put_char(c);
            break;
    }
}
