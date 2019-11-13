#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "tty.h"
#include "vga_buffer.h"

// #define XTERM_TTY_DEBUG

TTY::TTY(VgaBuffer& buffer)
    : m_buffer(buffer)
{
}

void TTY::draw(char c)
{
    if (m_col >= m_buffer.width()) {
        m_row++;
        m_col = 0;
    }

    m_buffer.draw(m_row, m_col++, c);

    update_cursor();
}

void TTY::update_cursor()
{
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
    int args[20] { 0 };
    bool starts_with_q = m_escape_buffer[1] == '?';

    int num_args = 0;
    for (int i = starts_with_q ? 2 : 1; i < m_escape_index - 1 && num_args < 20;) {
        char *next = nullptr;
        errno = 0;
        long res = strtol(m_escape_buffer + i, &next, 10);
        if (errno != 0) {
            return;
        }

        args[num_args++] = static_cast<int>(res);
        if (next[0] == ';') {
            i = next - m_escape_buffer + 1;
        } else {
            break;
        }
    }

#ifdef XTERM_TTY_DEBUG
    FILE* s = fopen("/dev/serial", "w");
    fprintf(s, "%s %d\n", m_escape_buffer, num_args);
    for (int i = 0; i < num_args; i++) {
        fprintf(s, "[%d]: [%d]\n", i, args[i]);
    }
    fclose(s);
#endif /* XTERM_TTY_DEBUG */

    switch (m_escape_buffer[m_escape_index - 1]) {
    case 'l':
        if (starts_with_q) {
            m_buffer.hide_cursor();
        }
        break;
    case 'h':
        if (starts_with_q) {
            m_buffer.show_cursor();
        }
        break;
    case 'A':
        m_row -= args[0];
        break;
    case 'B':
        m_row += args[0];
        break;
    case 'C':
        m_col += args[0];
        break;
    case 'D':
        m_col -= args[0];
        break;
    case 'H':
        m_row = args[0] - 1;
        m_col = args[1] - 1;
        break;
    case 'J':
        if (args[0] == 2) {
            m_buffer.clear();
        }
        break;
    case 'K':
        if (args[0] == 0) {
            m_buffer.clear_row_to_end(m_row, m_col);
        }
        if (args[0] == 2) {
            m_buffer.clear_row(m_row);
        }
        break;
    case 'm':
        for (int i = 0; i < num_args; i++) {
            switch (args[i]) {
            case 0:
                m_buffer.reset_colors();
                break;
            case 1:
                // Bold is not supported.
                break;
            case 7:
                m_buffer.swap_colors();
                break;
            case 30:
                m_buffer.set_fg(VGA_COLOR_BLACK);
                break;
            case 31:
                m_buffer.set_fg(VGA_COLOR_RED);
                break;
            case 32:
                m_buffer.set_fg(VGA_COLOR_GREEN);
                break;
            case 33:
                m_buffer.set_fg(VGA_COLOR_YELLOW);
                break;
            case 34:
                m_buffer.set_fg(VGA_COLOR_BLUE);
                break;
            case 35:
                m_buffer.set_fg(VGA_COLOR_MAGENTA);
                break;
            case 36:
                m_buffer.set_fg(VGA_COLOR_CYAN);
                break;
            case 37:
                m_buffer.set_fg(VGA_COLOR_LIGHT_GREY);
                break;
            case 39:
                m_buffer.reset_fg();
                break;
            case 40:
                m_buffer.set_bg(VGA_COLOR_BLACK);
                break;
            case 41:
                m_buffer.set_bg(VGA_COLOR_RED);
                break;
            case 42:
                m_buffer.set_bg(VGA_COLOR_GREEN);
                break;
            case 43:
                m_buffer.set_bg(VGA_COLOR_YELLOW);
                break;
            case 44:
                m_buffer.set_bg(VGA_COLOR_BLUE);
                break;
            case 45:
                m_buffer.set_bg(VGA_COLOR_MAGENTA);
                break;
            case 46:
                m_buffer.set_bg(VGA_COLOR_CYAN);
                break;
            case 47:
                m_buffer.set_bg(VGA_COLOR_LIGHT_GREY);
                break;
            case 49:
                m_buffer.reset_bg();
                break;
            default:
                break;
            }
        }
        break;
    case 's':
        save_pos();
        break;
    case 'u':
        restore_pos();
        break;
    default:
        break;
    }

    clamp_cursor();
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
    if (m_in_escape) {
        on_next_escape_char(c);
        return;
    }

    switch (c) {
    case '\033':
        m_in_escape = true;
        break;
    case '\r':
        m_col = 0;
        break;
    case '\n':
        m_row++;
        if (m_row >= m_buffer.height()) {
            m_buffer.scroll();
            m_row--;
        }
        break;
    default:
        draw(c);
        break;
    }
}