#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include "pseudo_terminal.h"
#include "tty.h"

// #define TERMINAL_DEBUG

void TTY::resize(int rows, int cols) {
    m_row_count = rows;
    m_col_count = cols;

    m_scroll_start = 0;
    m_scroll_end = rows - 1;

    m_rows.resize(rows);
    for (auto& row : m_rows) {
        row.resize(cols);
    }

    for (auto& row : m_rows_above) {
        row.resize(cols);
    }

    for (auto& row : m_rows_below) {
        row.resize(cols);
    }

    invalidate_all();
    clamp_cursor();
}

void TTY::invalidate_all() {
    for (auto& row : m_rows) {
        for (auto& cell : row) {
            cell.dirty = true;
        }
    }
}

void TTY::clear_below_cursor(char ch) {
    clear_row_to_end(m_cursor_row, m_cursor_col, ch);
    for (auto r = m_cursor_row + 1; r < m_row_count; r++) {
        clear_row(r, ch);
    }
}

void TTY::clear_above_cursor(char ch) {
    for (auto r = 0; r < m_cursor_row; r++) {
        clear_row(r, ch);
    }
    clear_row_until(m_cursor_row, m_cursor_col, ch);
}

void TTY::clear(char ch) {
    for (auto r = 0; r < m_row_count; r++) {
        clear_row(r, ch);
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
    if (iscntrl(c)) {
        put_char('^');
        put_char(c | 0x40);
        return;
    }

    if (m_x_overflow) {
        m_cursor_row++;
        scroll_down_if_needed();
        m_cursor_col = 0;
        m_x_overflow = false;
    }

    put_char(m_cursor_row, m_cursor_col, c);

    m_cursor_col++;
    if (m_cursor_col >= m_col_count) {
        m_x_overflow = true;
        m_cursor_col--;
    }
}

bool TTY::should_display_cursor_at_position(int r, int c) const {
    if (m_cursor_hidden) {
        return false;
    }

    if (c != m_cursor_col) {
        return false;
    }

    if (m_cursor_row < m_scroll_start || m_cursor_row > m_scroll_end || r < m_scroll_start || r > m_scroll_end) {
        return r == m_cursor_row;
    }

    return row_offset() + r == cursor_row() + total_rows() - row_count();
}

int TTY::scroll_relative_offset(int display_row) const {
    if (display_row < m_scroll_start) {
        return display_row;
    } else if (display_row > m_scroll_end) {
        return display_row + total_rows() - row_count();
    }
    return display_row + row_offset();
}

const TTY::Row& TTY::row_at_scroll_relative_offset(int offset) const {
    if (offset < m_scroll_start) {
        return m_rows[offset];
    }
    if (offset < m_scroll_start + m_rows_above.size()) {
        return m_rows_above[offset - m_scroll_start];
    }
    if (offset < m_scroll_start + m_rows_above.size() + (m_scroll_end - m_scroll_start)) {
        return m_rows[offset - m_rows_above.size()];
    }
    if (offset < m_scroll_start + m_rows_above.size() + (m_scroll_end - m_scroll_start) + m_rows_below.size()) {
        return m_rows_below[offset - m_scroll_start - m_rows_above.size() - (m_scroll_end - m_scroll_start)];
    }
    return m_rows[offset - m_rows_above.size() - m_rows_below.size()];
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
        m_x_overflow = false;
        m_cursor_hidden = false;
        m_cursor_row = m_cursor_col = m_saved_cursor_row = m_saved_cursor_col = 0;
        m_rows.resize(m_row_count);
        m_rows_above.clear();
        m_rows_below.clear();
        clear();
    } else {
        assert(m_save_state);
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
        m_rows = move(m_save_state->m_rows);
        m_rows_above = move(m_save_state->m_rows_above);
        m_rows_below = move(m_save_state->m_rows_below);

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

    if (m_escape_buffer[0] == ']') {
        return;
    }

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
                        preceding_character = m_rows[m_cursor_row - 1][m_col_count - 1].ch;
                    }
                } else {
                    preceding_character = m_rows[m_cursor_row][m_cursor_col - 1].ch;
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
                m_cursor_row = args.get_or(0, 1) - 1;
                return;
            case 'l':
                if (starts_with_q) {
                    switch (args.get_or(0, 0)) {
                        case 1:
                            m_psuedo_terminal.set_application_cursor_keys(false);
                            break;
                        case 9:
                            m_psuedo_terminal.reset_mouse_tracking_mode(MouseTrackingMode::X10);
                            break;
                        case 12:
                            // Stop cursor blink
                            break;
                        case 25:
                            m_cursor_hidden = true;
                            break;
                        case 1000:
                            m_psuedo_terminal.reset_mouse_tracking_mode(MouseTrackingMode::X11);
                            break;
                        case 1001:
                            m_psuedo_terminal.reset_mouse_tracking_mode(MouseTrackingMode::Hilite);
                            break;
                        case 1002:
                            m_psuedo_terminal.reset_mouse_tracking_mode(MouseTrackingMode::Cell);
                            break;
                        case 1003:
                            m_psuedo_terminal.reset_mouse_tracking_mode(MouseTrackingMode::All);
                            break;
                        case 1005:
                            if (m_psuedo_terminal.mouse_reporting_mode() == MouseReportingMode::UTF8) {
                                m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
                            }
                            break;
                        case 1006:
                            if (m_psuedo_terminal.mouse_reporting_mode() == MouseReportingMode::SGR) {
                                m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
                            }
                            break;
                        case 1015:
                            if (m_psuedo_terminal.mouse_reporting_mode() == MouseReportingMode::URXVT) {
                                m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
                            }
                            break;
                        case 1049:
                            set_use_alternate_screen_buffer(false);
                            break;
                        case 2004:
                            m_psuedo_terminal.set_bracketed_paste(false);
                            break;
                        default:
                            fprintf(stderr, "Unsupported DEC Reset %d\n", args.get_or(0, 0));
                            break;
                    }
                }
                return;
            case 'h':
                if (starts_with_q) {
                    if (starts_with_q) {
                        switch (args.get_or(0, 0)) {
                            case 1:
                                m_psuedo_terminal.set_application_cursor_keys(true);
                                break;
                            case 9:
                                m_psuedo_terminal.set_mouse_tracking_mode(MouseTrackingMode::X10);
                                m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X10);
                                break;
                            case 12:
                                // Start cursor blink
                                break;
                            case 25:
                                m_cursor_hidden = false;
                                break;
                            case 1000:
                                m_psuedo_terminal.set_mouse_tracking_mode(MouseTrackingMode::X10);
                                m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X10);
                                break;
                            case 1001:
                                m_psuedo_terminal.set_mouse_tracking_mode(MouseTrackingMode::X11);
                                m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
                                break;
                            case 1002:
                                m_psuedo_terminal.set_mouse_tracking_mode(MouseTrackingMode::Cell);
                                m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
                                break;
                            case 1003:
                                m_psuedo_terminal.set_mouse_tracking_mode(MouseTrackingMode::All);
                                m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::X11);
                                break;
                            case 1005:
                                if (m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::X11 ||
                                    m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::Cell ||
                                    m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::All) {
                                    m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::UTF8);
                                }
                                break;
                            case 1006:
                                if (m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::X11 ||
                                    m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::Cell ||
                                    m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::All) {
                                    m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::SGR);
                                }
                                break;
                            case 1015:
                                if (m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::X11 ||
                                    m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::Cell ||
                                    m_psuedo_terminal.mouse_tracking_mode() == MouseTrackingMode::All) {
                                    m_psuedo_terminal.set_mouse_reporting_mode(MouseReportingMode::URXVT);
                                }
                                break;
                            case 1049:
                                set_use_alternate_screen_buffer(true);
                                break;
                            case 2004:
                                m_psuedo_terminal.set_bracketed_paste(true);
                                break;
                            default:
                                fprintf(stderr, "Unsupported DEC Set %d\n", args.get_or(0, 0));
                                break;
                        }
                    }
                }
                return;
            case 'n':
                switch (args.get_or(0, 0)) {
                    case 5:
                        m_psuedo_terminal.write("\033[0n");
                        break;
                    case 6:
                        m_psuedo_terminal.write(String::format("\033[%d;%dR", m_cursor_row + 1, m_cursor_col + 1));
                        break;
                    default:
                        break;
                }
                return;
            case 'r': {
                int new_scroll_start = args.get_or(0, 1) - 1;
                int new_scroll_end = args.get_or(1, m_row_count) - 1;
                if (new_scroll_end - new_scroll_start < 2) {
                    return;
                }
                m_rows_above.clear();
                m_rows_below.clear();
                m_scroll_start = new_scroll_start;
                m_scroll_end = new_scroll_end;
                m_cursor_row = 0;
                m_cursor_col = 0;
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
                m_cursor_col = args.get_or(0, 1) - 1;
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
                    m_rows_above.clear();
                    m_rows_below.clear();
                    m_rows.resize(m_row_count);
                    clear();
                    return;
                }
                break;
            case 'K':
                if (args.get_or(0, 0) == 0) {
                    clear_row_to_end(m_cursor_row, m_cursor_col);
                    return;
                }
                if (args.get_or(0, 0) == 1) {
                    clear_row_until(m_cursor_row, m_cursor_col);
                    return;
                }
                if (args.get_or(0, 0) == 2) {
                    clear_row(m_cursor_row);
                    return;
                }
                break;
            case 'L': {
                if (m_cursor_row < m_scroll_start || m_cursor_row > m_scroll_end) {
                    return;
                }
                int lines_to_insert = args.get_or(0, 1);
                for (int i = 0; i < lines_to_insert; i++) {
                    m_rows.rotate_right(m_cursor_row, m_scroll_end + 1);
                    m_rows[m_cursor_row] = Row(m_col_count);
                    m_rows[m_cursor_row].resize(m_col_count);
                }
                invalidate_all();
                return;
            }
            case 'M': {
                if (m_cursor_row < m_scroll_start || m_cursor_row > m_scroll_end) {
                    return;
                }
                int lines_to_delete = clamp(args.get_or(0, 1), 1, m_scroll_end - m_cursor_row);
                for (int i = 0; i < lines_to_delete; i++) {
                    m_rows.rotate_left(m_cursor_row, m_scroll_end + 1);
                    m_rows[m_scroll_end] = Row(m_col_count);
                    m_rows[m_scroll_end].resize(m_col_count);
                }

                invalidate_all();
                return;
            }
            case 'P': {
                int chars_to_delete = clamp(args.get_or(0, 1), 1, m_col_count - m_cursor_col);
                for (int i = 0; i < chars_to_delete; i++) {
                    m_rows[m_cursor_row].remove(m_cursor_col);
                }
                m_rows[m_cursor_row].resize(m_col_count);
                for (int i = m_cursor_col; i < m_col_count; i++) {
                    m_rows[m_cursor_row][i].dirty = true;
                }
                return;
            }
            case 'S': {
                int to_scroll = args.get_or(0, 1);
                int row_save = m_cursor_row;
                for (int i = 0; i < to_scroll; i++) {
                    m_cursor_row = m_row_count;
                    scroll_down_if_needed();
                }
                m_cursor_row = row_save;
                return;
            }
            case 'T': {
                int to_scroll = args.get_or(0, 1);
                int row_save = m_cursor_row;
                for (int i = 0; i < to_scroll; i++) {
                    m_cursor_row = -1;
                    scroll_up_if_needed();
                }
                m_cursor_row = row_save;
                return;
            }
            case 'X': {
                int chars_to_erase = max(1, args.get_or(0, 1));
                for (int i = m_cursor_col; i < chars_to_erase && i < m_col_count; i++) {
                    m_rows[m_cursor_row][i] = Cell();
                }
                return;
            }
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
                scroll_down_if_needed();
                m_in_escape = false;
                return;
            case 'E':
                m_cursor_row++;
                m_cursor_col = 0;
                m_x_overflow = false;
                scroll_down_if_needed();
                m_in_escape = false;
                return;
            case 'M':
                m_cursor_row--;
                m_x_overflow = false;
                scroll_up_if_needed();
                m_in_escape = false;
                return;
            case '7':
                save_pos();
                m_in_escape = false;
                return;
            case '8':
                restore_pos();
                m_in_escape = false;
                return;
            case '=':
                m_in_escape = false;
                return;
            default:
                break;
        }
    }

    if (m_escape_index == 0 && (c != '[' && c != '#' && c != '(' && c != ']')) {
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
    if ((m_escape_buffer[0] != ']' && isalpha(c)) || (m_escape_buffer[0] == '#' && isdigit(c)) ||
        (m_escape_buffer[0] == ']' && (c == '\a' || c == '\0'))) {
        m_escape_buffer[m_escape_index] = '\0';
        handle_escape_sequence();
        m_escape_index = 0;
        m_in_escape = false;
    }
}

void TTY::scroll_up() {
    if (m_rows_above.empty()) {
        return;
    }

    m_rows.rotate_right(m_scroll_start, m_scroll_end + 1);
    m_rows_below.add(move(m_rows[m_scroll_start]));
    m_rows[m_scroll_start] = move(m_rows_above.last());
    m_rows_above.remove_last();
    invalidate_all();
}

void TTY::scroll_down() {
    if (m_rows_below.empty()) {
        return;
    }

    m_rows.rotate_left(m_scroll_start, m_scroll_end + 1);
    m_rows_above.add(move(m_rows[m_scroll_end]));
    m_rows[m_scroll_end] = move(m_rows_below.last());
    m_rows_below.remove_last();
    invalidate_all();
}

void TTY::scroll_up_if_needed() {
    if (m_cursor_row == m_scroll_start - 1 || m_cursor_row == m_scroll_end + 1) {
        m_cursor_row = clamp(m_cursor_row, m_scroll_start, m_scroll_end);
        m_x_overflow = false;

        if (!m_rows_above.empty()) {
            scroll_up();
            return;
        }

        m_rows.rotate_right(m_scroll_start, m_scroll_end + 1);
        m_rows_below.add(move(m_rows[m_scroll_start]));
        m_rows[m_scroll_start] = Row(m_col_count);
        m_rows[m_scroll_start].resize(m_col_count);
        invalidate_all();

        if (total_rows() - m_rows.size() > m_row_count + 100) {
            m_rows_below.remove(0);
        }
    }
}

void TTY::scroll_down_if_needed() {
    if (m_cursor_row == m_scroll_start - 1 || m_cursor_row == m_scroll_end + 1) {
        m_cursor_row = clamp(m_cursor_row, m_scroll_start, m_scroll_end);
        m_x_overflow = false;

        if (!m_rows_below.empty()) {
            scroll_down();
            return;
        }

        m_rows.rotate_left(m_scroll_start, m_scroll_end + 1);
        m_rows_above.add(move(m_rows[m_scroll_end]));
        m_rows[m_scroll_end] = Row(m_col_count);
        m_rows[m_scroll_end].resize(m_col_count);
        invalidate_all();

        if (total_rows() - m_rows.size() > m_row_count + 100) {
            m_rows_above.remove(0);
        }
    }
}

void TTY::scroll_to_bottom() {
    while (!m_rows_below.empty()) {
        scroll_down();
    }
}

void TTY::on_char(char c) {
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
        case '\a':
            // Ignore alarm character for now
            break;
        case 127:
            // Ignore this for now (this should definitely not be printed)
            break;
        default:
            put_char(c);
            break;
    }
}
