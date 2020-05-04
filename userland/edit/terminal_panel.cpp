#include <assert.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "editor.h"
#include "terminal_panel.h"

static termios s_original_termios;
static bool s_raw_mode_enabled;

static void restore_termios() {
    tcsetattr(STDOUT_FILENO, TCSAFLUSH, &s_original_termios);

    // FIXME: it would be nice to somehow restore the old terminal
    //        state instead of just clearing everything.
    fputs("\033[1;1H\033[2J", stdout);
}

static void enable_raw_mode() {
    s_raw_mode_enabled = true;

    assert(tcgetattr(STDOUT_FILENO, &s_original_termios) == 0);

    termios to_set = s_original_termios;

    // Raw mode flags according to `man tcsetattr (GNU/Linux)`
    to_set.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    to_set.c_oflag &= ~OPOST;
    to_set.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    to_set.c_cflag &= ~(CSIZE | PARENB);
    to_set.c_cflag |= CS8;

    assert(tcsetattr(STDOUT_FILENO, TCSAFLUSH, &to_set) == 0);

    atexit(restore_termios);

    setvbuf(stdout, nullptr, _IOFBF, BUFSIZ);
}

TerminalPanel::TerminalPanel() {
    assert(isatty(STDOUT_FILENO));

    if (!s_raw_mode_enabled) {
        enable_raw_mode();
    }

    winsize sz;
    assert(ioctl(STDOUT_FILENO, TIOCGWINSZ, &sz) == 0);
    m_rows = sz.ws_row;
    m_cols = sz.ws_col;
    m_chars.resize(m_rows * m_cols);
    assert(m_chars.size() == m_rows * m_cols);
}

TerminalPanel::~TerminalPanel() {}

void TerminalPanel::clear() {
    fputs("\033[2J", stdout);
    draw_cursor();
    memset(m_chars.vector(), 0, m_chars.size());
}

void TerminalPanel::draw_cursor() {
    printf("\033[%d;%dH", m_cursor_row + 1, m_cursor_col + 1);
    fflush(stdout);
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
        fputc('\r', stdout);
        fputc('\n', stdout);
    }
}

void TerminalPanel::flush() {
    fputs("\033[1;1H", stdout);
    for (int r = 0; r < rows(); r++) {
        flush_row(r);
    }
    draw_cursor();
}

KeyPress TerminalPanel::read_key() {
    char ch;
    assert(read(STDIN_FILENO, &ch, 1) > 0);

    if (ch == '\033') {
        char escape_buffer[40];
        assert(read(STDIN_FILENO, escape_buffer, 1) > 0);

        assert(escape_buffer[0] == '[');

        assert(read(STDIN_FILENO, escape_buffer + 1, 1) > 0);
        switch (escape_buffer[1]) {
            case 'A':
                return { 0, KeyPress::Key::UpArrow };
            case 'B':
                return { 0, KeyPress::Key::DownArrow };
            case 'C':
                return { 0, KeyPress::Key::RightArrow };
            case 'D':
                return { 0, KeyPress::Key::LeftArrow };
        }
    }

    return { 0, ch };
}

void TerminalPanel::enter() {
    for (;;) {
        KeyPress press = read_key();
        if (press.key == 'q') {
            exit(0);
        }

        if (auto* document = Panel::document()) {
            document->notify_key_pressed(press);
        }
    }
}