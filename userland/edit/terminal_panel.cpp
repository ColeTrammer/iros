#include <assert.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "terminal_panel.h"

TerminalPanel::TerminalPanel() {
    assert(isatty(STDOUT_FILENO));

    winsize sz;
    assert(ioctl(STDOUT_FILENO, TIOCGWINSZ, &sz) == 0);
    m_rows = sz.ws_row;
    m_cols = sz.ws_col;
    m_chars.resize(m_rows * m_cols);
    assert(m_chars.size() == m_rows * m_cols);
}

TerminalPanel::~TerminalPanel() {}

void TerminalPanel::clear() {
    fputs("\033[3J", stdout);
    draw_cursor();
}

void TerminalPanel::draw_cursor() {
    printf("\033[%d;%dH", m_cursor_row + 1, m_cursor_col + 1);
}

void TerminalPanel::set_text_at(int row, int col, char c) {
    m_chars[index(row, col)] = c;
}

void TerminalPanel::set_cursor(int row, int col) {
    if (m_cursor_row == row && m_cursor_col == col) {
        return;
    }

    m_cursor_row = row;
    m_cursor_col = col;
    draw_cursor();
}

void TerminalPanel::print_char(char c) {
    if (c == '\0') {
        c = ' ';
    }

    fputc(c, stdout);
}

void TerminalPanel::flush_row(int row) {
    for (int c = 0; c < m_cols; c++) {
        print_char(m_chars[index(row, c)]);
    }

    if (row != rows() - 1) {
        fputc('\n', stdout);
    }
}

void TerminalPanel::flush() {
    fputs("\033[0;0H", stdout);
    for (int r = 0; r < rows(); r++) {
        flush_row(r);
    }
    draw_cursor();
}