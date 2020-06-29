#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include "tty.h"

void TTY::resize(int rows, int cols) {
    m_row_count = rows;
    m_col_count = cols;

    m_row_offset = 0;

    m_rows.resize(rows);
    for (auto& row : m_rows) {
        row.resize(cols);
    }

    invalidate_all();

    clamp_cursor();
    m_cursor_row = min(m_cursor_row, rows - 1);
    m_cursor_col = min(m_cursor_col, cols - 1);
}

void TTY::invalidate_all() {
    for (auto& row : m_rows) {
        for (auto& cell : row) {
            cell.dirty = true;
        }
    }
}

void TTY::clear() {
    for (auto r = 0; r < m_row_count; r++) {
        clear_row(r + m_row_offset);
    }
}

void TTY::clear_row(int r) {
    clear_row_to_end(r, 0);
}

void TTY::clear_row_to_end(int r, int start_col) {
    for (auto c = start_col; c < m_col_count; c++) {
        put_char(r, c, ' ');
    }
}

void TTY::put_char(int row, int col, char c) {
    auto& cell = m_rows[row][col];
    cell.ch = c;
    cell.bg = m_bg;
    cell.fg = m_fg;
    cell.bold = m_bold;
    cell.inverted = m_inverted;
    cell.dirty = true;
}

void TTY::put_char(char c) {
    put_char(m_cursor_row + m_row_offset, m_cursor_col, c);

    m_cursor_col++;
    if (m_cursor_col >= m_col_count) {
        m_cursor_col--;
    }
}

void TTY::clamp_cursor() {
    m_cursor_row = clamp(m_cursor_row, 0, m_row_count - 1);
    m_cursor_col = clamp(m_cursor_col, 0, m_col_count - 1);
}

void TTY::handle_escape_sequence() {
    LIIM::Vector<int> args;
    bool starts_with_q = m_escape_buffer[1] == '?';

    for (int i = starts_with_q ? 2 : 1; i < m_escape_index - 1;) {
        char* next = nullptr;
        errno = 0;
        long res = strtol(m_escape_buffer + i, &next, 10);
        if (errno != 0) {
            return;
        }

        args.add(static_cast<int>(res));
        if (next[0] == ';') {
            i = next - m_escape_buffer + 1;
        } else {
            break;
        }
    }

    switch (m_escape_buffer[m_escape_index - 1]) {
        case 'l':
            if (starts_with_q) {
                m_cursor_hidden = true;
            }
            return;
        case 'h':
            if (starts_with_q) {
                m_cursor_hidden = false;
            }
            return;
        case 'A':
            if (args.size() != 1) {
                break;
            }
            m_cursor_row -= args.get(0);
            return;
        case 'B':
            if (args.size() != 1) {
                break;
            }
            m_cursor_row += args.get(0);
            return;
        case 'C':
            if (args.size() != 1) {
                break;
            }
            m_cursor_col += args.get(0);
            return;
        case 'D':
            if (args.size() != 1) {
                break;
            }
            m_cursor_col -= args.get(0);
            return;
        case 'H':
            m_cursor_row = args.get_or(0, 1) - 1;
            m_cursor_col = args.get_or(1, 1) - 1;
            return;
        case 'J':
            if (args.get_or(0, 0) == 2) {
                clear();
                return;
            } else if (args.get_or(0, 0) == 3) {
                m_row_offset = 0;
                m_rows.resize(m_row_count);
                clear();
                return;
            }
            break;
        case 'K':
            if (args.get_or(0, 0) == 0) {
                clear_row_to_end(m_cursor_row + m_row_offset, m_cursor_col);
                return;
            }
            if (args.get_or(0, 0) == 2) {
                clear_row(m_cursor_row + m_row_offset);
                return;
            }
            break;
        case 'm':
            if (args.size() == 0) {
                args.add(0);
            }
            for (int i = 0; i < args.size(); i++) {
                switch (args[i]) {
                    case 0:
                        reset_attributes();
                        break;
                    case 1:
                        set_bold(true);
                        break;
                    case 7:
                        set_inverted(true);
                        break;
                    case 30:
                        set_fg(VGA_COLOR_BLACK);
                        break;
                    case 31:
                        set_fg(VGA_COLOR_RED);
                        break;
                    case 32:
                        set_fg(VGA_COLOR_GREEN);
                        break;
                    case 33:
                        set_fg(VGA_COLOR_BROWN);
                        break;
                    case 34:
                        set_fg(VGA_COLOR_BLUE);
                        break;
                    case 35:
                        set_fg(VGA_COLOR_MAGENTA);
                        break;
                    case 36:
                        set_fg(VGA_COLOR_CYAN);
                        break;
                    case 37:
                        set_fg(VGA_COLOR_LIGHT_GREY);
                        break;
                    case 39:
                        reset_fg();
                        break;
                    case 40:
                        set_bg(VGA_COLOR_BLACK);
                        break;
                    case 41:
                        set_bg(VGA_COLOR_RED);
                        break;
                    case 42:
                        set_bg(VGA_COLOR_GREEN);
                        break;
                    case 43:
                        set_bg(VGA_COLOR_BROWN);
                        break;
                    case 44:
                        set_bg(VGA_COLOR_BLUE);
                        break;
                    case 45:
                        set_bg(VGA_COLOR_MAGENTA);
                        break;
                    case 46:
                        set_bg(VGA_COLOR_CYAN);
                        break;
                    case 47:
                        set_bg(VGA_COLOR_LIGHT_GREY);
                        break;
                    case 49:
                        reset_bg();
                        break;
                    case 90:
                        set_fg(VGA_COLOR_DARK_GREY);
                        break;
                    case 91:
                        set_fg(VGA_COLOR_LIGHT_RED);
                        break;
                    case 92:
                        set_fg(VGA_COLOR_LIGHT_GREEN);
                        break;
                    case 93:
                        set_fg(VGA_COLOR_YELLOW);
                        break;
                    case 94:
                        set_fg(VGA_COLOR_LIGHT_BLUE);
                        break;
                    case 95:
                        set_fg(VGA_COLOR_LIGHT_MAGENTA);
                        break;
                    case 96:
                        set_fg(VGA_COLOR_LIGHT_CYAN);
                        break;
                    case 97:
                        set_fg(VGA_COLOR_WHITE);
                        break;
                    case 100:
                        set_bg(VGA_COLOR_DARK_GREY);
                        break;
                    case 101:
                        set_bg(VGA_COLOR_LIGHT_RED);
                        break;
                    case 102:
                        set_bg(VGA_COLOR_LIGHT_GREEN);
                        break;
                    case 103:
                        set_bg(VGA_COLOR_YELLOW);
                        break;
                    case 104:
                        set_bg(VGA_COLOR_LIGHT_BLUE);
                        break;
                    case 105:
                        set_bg(VGA_COLOR_LIGHT_MAGENTA);
                        break;
                    case 106:
                        set_bg(VGA_COLOR_LIGHT_CYAN);
                        break;
                    case 107:
                        set_bg(VGA_COLOR_WHITE);
                        break;
                    default:
                        break;
                }
            }
            return;
        case 's':
            save_pos();
            return;
        case 'u':
            restore_pos();
            return;
        default:
            break;
    }

    m_in_escape = false;
    int save_length = m_escape_index;
    m_escape_index = 0;
    put_char('^');
    put_char('[');
    for (int i = 0; i < save_length; i++) {
        on_char(m_escape_buffer[i]);
    }
}

void TTY::on_next_escape_char(char c) {
    assert(m_in_escape);

    if (m_escape_index == 0 && c != '[') {
        m_escape_index = 0;
        m_in_escape = false;
        return;
    }

    if (m_escape_index + 1 >= (int) sizeof(m_escape_buffer)) {
        m_escape_index = 0;
        m_in_escape = false;
        return;
    }

    m_escape_buffer[m_escape_index++] = c;
    if (isalpha(c)) {
        m_escape_buffer[m_escape_index] = '\0';
        handle_escape_sequence();
        m_escape_index = 0;
        m_in_escape = false;
    }
}

void TTY::scroll_down_if_needed() {
    if (m_cursor_row >= m_row_count) {
        m_row_offset++;
        m_cursor_row--;
        invalidate_all();
        m_rows.add(Row());
        m_rows.last().resize(m_col_count);

        if (m_rows.size() > m_row_count + 100) {
            m_rows.remove(0);
        }
    }
}

void TTY::on_char(char c) {
    if (m_in_escape) {
        on_next_escape_char(c);
        clamp_cursor();
        return;
    }

    switch (c) {
        case '\0':
            return;
        case '\033':
            m_in_escape = true;
            break;
        case '\r':
            m_cursor_col = 0;
            break;
        case '\n':
            m_cursor_row++;
            scroll_down_if_needed();
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
