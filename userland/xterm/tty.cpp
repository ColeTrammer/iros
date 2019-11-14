#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector.h>

#include "tty.h"
#include "vga_buffer.h"

// #define XTERM_TTY_DEBUG

#define CTRL_KEY(c) ((c) & 0x1F)
#define IS_CTRL(c) (!((c) & ~0x1F))

TTY::TTY(VgaBuffer& buffer)
    : m_buffer(buffer)
{
}

void TTY::scroll_up()
{
    if (m_above_rows.size() == 0) {
        return;
    }

    m_below_rows.add(m_buffer.scroll_down(m_above_rows.last()));
    delete[] m_above_rows.last();
    m_above_rows.remove_last();

    update_cursor();
}

void TTY::scroll_down()
{
    if (m_below_rows.size() == 0) {
        return;
    }

    m_above_rows.add(m_buffer.scroll_up(m_below_rows.last()));
    delete[] m_below_rows.last();
    m_below_rows.remove_last();

    update_cursor();
}

void TTY::scroll_to_bottom()
{
    while (m_below_rows.size() > 0) {
        scroll_down();
    }
}

void TTY::scroll_to_top()
{
    while (m_above_rows.size() > 0) {
        scroll_up();
    }
}

void TTY::draw(char c)
{
    if (IS_CTRL(c)) {
        draw('^');
        draw(c | 0x40);
        return;
    }

    if (m_col >= m_buffer.width()) {
        m_row++;
        if (m_row >= m_buffer.height()) {
            m_above_rows.add(m_buffer.scroll_up());
            m_row--;
        }
        m_col = 0;
    }

    m_buffer.draw(m_row, m_col++, c);
}

void TTY::update_cursor()
{
    if (m_below_rows.size() == 0 && !m_cursor_hidden) {
        m_buffer.show_cursor();
    } else {
        m_buffer.hide_cursor();
    }
    m_buffer.set_cursor(m_row, m_col);
}

void TTY::clamp_cursor()
{
    if (m_row < 0) {
        m_row = 0;
    }
    if (m_row >= m_buffer.height()) {
        m_row = m_buffer.height() - 1;
    }
    if (m_col < 0) {
        m_col = 0;
    }
    if (m_col >= m_buffer.width()) {
        m_col = m_buffer.width() - 1;
    }
}

void TTY::handle_escape_sequence()
{
    LIIM::Vector<int> args;
    bool starts_with_q = m_escape_buffer[1] == '?';

    for (int i = starts_with_q ? 2 : 1; i < m_escape_index - 1;) {
        char *next = nullptr;
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

#ifdef XTERM_TTY_DEBUG
    FILE* s = fopen("/dev/serial", "w");
    fprintf(s, "%s %d\n", m_escape_buffer, args.size());
    for (int i = 0; i < args.size(); i++) {
        fprintf(s, "[%d]: [%d]\n", i, args[i]);
    }
    fclose(s);
#endif /* XTERM_TTY_DEBUG */

    switch (m_escape_buffer[m_escape_index - 1]) {
    case 'l':
        if (starts_with_q) {
            m_cursor_hidden = true;
            m_buffer.hide_cursor();
        }
        return;
    case 'h':
        if (starts_with_q) {
            m_cursor_hidden = false;
            m_buffer.show_cursor();
        }
        return;
    case 'A':
        if (args.size() != 1) {
            break;
        }
        m_row -= args.get(0);
        return;
    case 'B':
        if (args.size() != 1) {
            break;
        }
        m_row += args.get(0);
        return;
    case 'C':
        if (args.size() != 1) {
            break;
        }
        m_col += args.get(0);
        return;
    case 'D':
        if (args.size() != 1) {
            break;
        }
        m_col -= args.get(0);
        return;
    case 'H':
        m_row = args.get_or(0, 1) - 1;
        m_col = args.get_or(1, 1) - 1;
        return;
    case 'J':
        if (args.get_or(0, 0) == 2) {
            m_buffer.clear();
            return;
        } else if (args.get_or(0, 0) == 3) {
            while (m_above_rows.size() > 0) {
                delete[] m_above_rows.last();
                m_above_rows.remove_last();
            }
            while (m_below_rows.size() > 0) {
                delete[] m_below_rows.last();
                m_below_rows.remove_last();
            }
            m_buffer.clear();
            return;
        }
        break;
    case 'K':
        if (args.get_or(0, 0) == 0) {
            m_buffer.clear_row_to_end(m_row, m_col);
            return;
        }
        if (args.get_or(0, 0) == 2) {
            m_buffer.clear_row(m_row);
            return;
        }
        break;
    case 'm':
        for (int i = 0; i < args.size(); i++) {
            switch (args[i]) {
            case 0:
                m_buffer.reset_colors();
                return;
            case 1:
                // Bold is not supported.
                break;
            case 7:
                m_buffer.swap_colors();
                return;
            case 30:
                m_buffer.set_fg(VGA_COLOR_BLACK);
                return;
            case 31:
                m_buffer.set_fg(VGA_COLOR_RED);
                return;
            case 32:
                m_buffer.set_fg(VGA_COLOR_GREEN);
                return;
            case 33:
                m_buffer.set_fg(VGA_COLOR_YELLOW);
                return;
            case 34:
                m_buffer.set_fg(VGA_COLOR_BLUE);
                return;
            case 35:
                m_buffer.set_fg(VGA_COLOR_MAGENTA);
                return;
            case 36:
                m_buffer.set_fg(VGA_COLOR_CYAN);
                return;
            case 37:
                m_buffer.set_fg(VGA_COLOR_LIGHT_GREY);
                return;
            case 39:
                m_buffer.reset_fg();
                return;
            case 40:
                m_buffer.set_bg(VGA_COLOR_BLACK);
                return;
            case 41:
                m_buffer.set_bg(VGA_COLOR_RED);
                return;
            case 42:
                m_buffer.set_bg(VGA_COLOR_GREEN);
                return;
            case 43:
                m_buffer.set_bg(VGA_COLOR_YELLOW);
                return;
            case 44:
                m_buffer.set_bg(VGA_COLOR_BLUE);
                return;
            case 45:
                m_buffer.set_bg(VGA_COLOR_MAGENTA);
                return;
            case 46:
                m_buffer.set_bg(VGA_COLOR_CYAN);
                return;
            case 47:
                m_buffer.set_bg(VGA_COLOR_LIGHT_GREY);
                return;
            case 49:
                m_buffer.reset_bg();
                return;
            default:
                break;
            }
        }
        break;
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
    draw('^');
    draw('[');
    for (int i = 0; i < save_length; i++) {
        on_char(m_escape_buffer[i]);
    }
}

void TTY::on_next_escape_char(char c)
{
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
        update_cursor();
        m_escape_index = 0;
        m_in_escape = false;
    }
}

void TTY::on_char(char c)
{
    scroll_to_bottom();

    if (m_in_escape) {
        on_next_escape_char(c);
        clamp_cursor();
        update_cursor();
        return;
    }

    switch (c) {
    case CTRL_KEY('d'):
        break;
    case '\033':
        m_in_escape = true;
        break;
    case '\r':
        m_col = 0;
        break;
    case '\n':
        m_row++;
        if (m_row >= m_buffer.height()) {
            m_above_rows.add(m_buffer.scroll_up());
            m_row--;
        }
        break;
    case 127:
        m_col--;
        draw(' ');
        m_col--;
        break;
    default:
        draw(c);
        break;
    }

    update_cursor();
}