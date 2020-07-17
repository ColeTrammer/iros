#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include "pseudo_terminal.h"
#include "tty.h"

// #define TERMINAL_DEBUG

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

void TTY::clear_below_cursor(char ch) {
    clear_row_to_end(m_row_offset + m_cursor_row, m_cursor_col, ch);
    for (auto r = m_row_offset + m_cursor_row + 1; r < m_row_offset + m_row_count; r++) {
        clear_row(r, ch);
    }
}

void TTY::clear_above_cursor(char ch) {
    for (auto r = m_row_offset; r < m_row_offset + m_cursor_row; r++) {
        clear_row(r, ch);
    }
    clear_row_until(m_row_offset + m_cursor_row, m_cursor_col, ch);
}

void TTY::clear(char ch) {
    for (auto r = 0; r < m_row_count; r++) {
        clear_row(r + m_row_offset, ch);
    }
}

void TTY::clear_row(int r, char ch) {
    clear_row_to_end(r, 0, ch);
}

void TTY::clear_row_until(int r, int end_col, char ch) {
    for (auto c = 0; c <= end_col; c++) {
        put_char(r, c, ch);
    }
}

void TTY::clear_row_to_end(int r, int start_col, char ch) {
    for (auto c = start_col; c < m_col_count; c++) {
        put_char(r, c, ch);
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
    if (m_x_overflow) {
        m_cursor_row++;
        scroll_down_if_needed();
        m_cursor_col = 0;
        m_x_overflow = false;
    }

    put_char(m_cursor_row + m_row_offset, m_cursor_col, c);

    m_cursor_col++;
    if (m_cursor_col >= m_col_count) {
        m_x_overflow = true;
        m_cursor_col--;
    }
}

void TTY::clamp_cursor() {
    m_cursor_row = clamp(m_cursor_row, 0, m_row_count - 1);
    m_cursor_col = clamp(m_cursor_col, 0, m_col_count - 1);
}

void TTY::set_use_alternate_screen_buffer(bool b) {
    if ((!b && !m_save_state) || (b && m_save_state)) {
        return;
    }

    if (b) {
        m_save_state = make_shared<TTY>(*this);
        reset_attributes();
        m_row_offset = 0;
        m_x_overflow = false;
        m_cursor_hidden = false;
        m_cursor_row = m_cursor_col = m_saved_cursor_row = m_saved_cursor_col = 0;
        m_rows.resize(m_row_count);
        clear();
    } else {
        assert(m_save_state);
        m_row_offset = m_save_state->m_row_offset;
        m_cursor_row = m_save_state->m_cursor_row;
        m_cursor_col = m_save_state->m_cursor_col;
        m_saved_cursor_row = m_save_state->m_saved_cursor_row;
        m_saved_cursor_col = m_save_state->m_saved_cursor_col;
        m_bold = m_save_state->m_bold;
        m_inverted = m_save_state->m_inverted;
        m_bg = m_save_state->m_bg;
        m_fg = m_save_state->m_fg;
        m_x_overflow = m_save_state->m_x_overflow;
        m_cursor_hidden = m_save_state->m_cursor_hidden;
        m_rows = m_save_state->m_rows;

        if (m_row_count != m_save_state->m_row_count || m_col_count != m_save_state->m_col_count) {
            resize(m_row_count, m_col_count);
        } else {
            invalidate_all();
        }

        m_save_state = nullptr;
    }
}

void TTY::handle_escape_sequence() {
    LIIM::Vector<int> args;
    bool starts_with_q = m_escape_buffer[1] == '?';
    bool starts_with_hashtag = m_escape_buffer[0] == '#';
    bool starts_with_lparen = m_escape_buffer[0] == '(';

#ifdef TERMINAL_DEBUG
    fprintf(stderr, "^[%s\n", m_escape_buffer);
#endif /* TERMINAL_DEBUG */

    for (int i = starts_with_q ? 2 : 1; i < m_escape_index - !starts_with_hashtag;) {
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

    if (starts_with_lparen) {
        // FIXME: actually deal with the charset things ESC ( deals with.
        return;
    }

    if (starts_with_hashtag) {
        if (args.size() == 1) {
            switch (args.get(0)) {
                case 8:
                    clear('E');
                    return;
            }
        }
    } else {
        switch (m_escape_buffer[m_escape_index - 1]) {
            case 'b': {
                char preceding_character = ' ';
                if (m_cursor_col == 0) {
                    if (m_cursor_row != 0) {
                        preceding_character = m_rows[m_cursor_row + m_row_offset - 1][m_col_count - 1].ch;
                    }
                } else {
                    preceding_character = m_rows[m_cursor_row + m_row_offset][m_cursor_col - 1].ch;
                }
                for (int i = 0; i < args.get_or(0, 0); i++) {
                    put_char(preceding_character);
                }
                return;
            }
            case 'c':
                if (args.get_or(0, 0) != 0) {
                    break;
                }
                m_psuedo_terminal.write("\033[?1;0c");
                return;
            case 'd':
                m_cursor_row = args.get_or(0, 0) + 1;
                return;
            case 'l':
                if (starts_with_q) {
                    switch (args.get_or(0, 0)) {
                        case 25:
                            m_cursor_hidden = true;
                            break;
                        case 1049:
                            set_use_alternate_screen_buffer(false);
                        default:
                            break;
                    }
                }
                return;
            case 'h':
                if (starts_with_q) {
                    if (starts_with_q) {
                        switch (args.get_or(0, 0)) {
                            case 25:
                                m_cursor_hidden = false;
                                break;
                            case 1049:
                                set_use_alternate_screen_buffer(true);
                                break;
                            default:
                                break;
                        }
                    }
                }
                return;
            case 'r': {
                int region_start = args.get_or(0, 1) - 1;
                int region_end = args.get_or(1, m_row_count) - 1;
                fprintf(stderr, "Should set scrolling region to <%d, %d>\n", region_start, region_end);
                return;
            }
            case 't':
                fprintf(stderr, "Window controls %d;%d;%d\n", args.get_or(0, 0), args.get_or(1, 0), args.get_or(2, 0));
                return;
            case 'A':
                m_cursor_row -= max(1, args.get_or(0, 1));
                m_x_overflow = false;
                return;
            case 'B':
                m_cursor_row += max(1, args.get_or(0, 1));
                m_x_overflow = false;
                return;
            case 'C':
                m_cursor_col += max(1, args.get_or(0, 1));
                m_x_overflow = false;
                return;
            case 'D':
                m_cursor_col -= max(1, args.get_or(0, 1));
                m_x_overflow = false;
                return;
            case 'H':
            case 'f':
                m_cursor_row = args.get_or(0, 1) - 1;
                m_cursor_col = args.get_or(1, 1) - 1;
                m_x_overflow = false;
                return;
            case 'G':
                m_cursor_col = args.get_or(0, 0) + 1;
                return;
            case 'J':
                if (args.get_or(0, 0) == 0) {
                    clear_below_cursor();
                    return;
                }
                if (args.get_or(0, 0) == 1) {
                    clear_above_cursor();
                    return;
                }
                if (args.get_or(0, 0) == 2) {
                    clear();
                    return;
                }
                if (args.get_or(0, 0) == 3) {
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
                if (args.get_or(0, 0) == 1) {
                    clear_row_until(m_cursor_row + m_row_offset, m_cursor_col);
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
    }

    fprintf(stderr, "UNHANDLED ESCAPE: ^[%s\n", m_escape_buffer);

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

    if (m_escape_index == 0) {
        switch (c) {
            case 'D':
                m_cursor_row++;
                m_x_overflow = 0;
                m_in_escape = false;
                return;
            case 'E':
                m_cursor_row++;
                m_cursor_col = 0;
                m_x_overflow = false;
                m_in_escape = false;
                return;
            case 'M':
                m_cursor_row--;
                m_x_overflow = false;
                m_in_escape = false;
                break;
            default:
                break;
        }
    }

    if (m_escape_index == 0 && (c != '[' && c != '#' && c != '(')) {
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
    if (isalpha(c) || (m_escape_buffer[0] == '#' && isdigit(c))) {
        m_escape_buffer[m_escape_index] = '\0';
        handle_escape_sequence();
        m_escape_index = 0;
        m_in_escape = false;
    }
}

void TTY::scroll_up() {
    if (m_row_offset == 0) {
        return;
    }

    m_row_offset--;
    invalidate_all();
}

void TTY::scroll_down() {
    if (m_row_offset + m_row_count >= m_rows.size()) {
        return;
    }

    m_row_offset++;
    invalidate_all();
}

void TTY::scroll_down_if_needed() {
    if (m_cursor_row >= m_row_count) {
        m_row_offset++;
        m_cursor_row--;
        m_x_overflow = false;
        invalidate_all();
        m_rows.add(Row());
        m_rows.last().resize(m_col_count);

        if (m_rows.size() > m_row_count + 100) {
            m_rows.remove(0);
            m_row_offset--;
        }
    }
}

void TTY::scroll_to_bottom() {
    if (m_row_offset == m_rows.size() - m_row_count) {
        return;
    }

    m_row_offset = m_rows.size() - m_row_count;
    invalidate_all();
}

void TTY::on_char(char c) {
    scroll_to_bottom();

#if 0
    if (c != '\033') {
        fprintf(stderr, "on_char(%3d, '%c')\n", c, c);
    } else {
        fprintf(stderr, "on_char(%3d, <ESC>)\n", 033);
    }
#endif /* 0 */

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
            m_x_overflow = false;
            break;
        case '\n':
            m_cursor_row++;
            scroll_down_if_needed();
            m_x_overflow = false;
            break;
        // Ascii BS (NOTE: not the backspace key)
        case 8:
            if (m_cursor_col > 0) {
                m_cursor_col--;
            }
            m_x_overflow = false;
            break;
        // Ascii DEL (NOTE: not the delete key)
        case 127:
            if (m_cursor_col > 0) {
                m_cursor_col--;
                put_char(' ');
                m_cursor_col--;
            }
            m_x_overflow = false;
            break;
        case '\a':
            // Ignore alarm character for now
            break;
        default:
            put_char(c);
            break;
    }
}
