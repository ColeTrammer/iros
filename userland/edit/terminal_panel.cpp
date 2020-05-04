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

constexpr int status_bar_height = 1;
constexpr time_t status_message_timeout = 3;

TerminalPanel::TerminalPanel() {
    assert(isatty(STDOUT_FILENO));

    if (!s_raw_mode_enabled) {
        enable_raw_mode();
    }

    winsize sz;
    assert(ioctl(STDOUT_FILENO, TIOCGWINSZ, &sz) == 0);
    m_rows = sz.ws_row - status_bar_height;
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

void TerminalPanel::draw_status_message() {
    if (m_status_message.is_empty()) {
        return;
    }

    fputs("\033[s", stdout);
    printf("\033[%d;1H", m_rows + 1);

    if (time(nullptr) - m_status_message_time > status_message_timeout) {
        m_status_message = "";
        fputs("\033[0J", stdout);
    } else {
        fputs(m_status_message.string(), stdout);
    }

    fputs("\033[u", stdout);
    fflush(stdout);
}

void TerminalPanel::send_status_message(String message) {
    m_status_message = move(message);
    m_status_message_time = time(nullptr);
    draw_status_message();
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
    fputs("\033[?l", stdout);
    fputs("\033[1;1H", stdout);
    for (int r = 0; r < rows(); r++) {
        flush_row(r);
    }
    fputs("\033[?h", stdout);
    draw_status_message();
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
            case 'F':
                return { 0, KeyPress::Key::End };
            case 'H':
                return { 0, KeyPress::Key::Home };
            case '3':
                assert(read(STDIN_FILENO, escape_buffer + 2, 1) > 0);
                switch (escape_buffer[2]) {
                    case '~':
                        return { 0, KeyPress::Key::Delete };
                }
        }
    }

    if (ch == 127) {
        return { 0, KeyPress::Key::Backspace };
    }

    if (ch == '\r') {
        return { 0, KeyPress::Key::Enter };
    }

    if (ch >= ('a' & 0x1F) && ch <= ('z' & 0x1F)) {
        return { KeyPress::Modifier::Control, ch | 0b1000000 };
    }

    return { 0, ch };
}

void TerminalPanel::enter() {
    for (;;) {
        KeyPress press = read_key();
        if (auto* document = Panel::document()) {
            document->notify_key_pressed(press);
        }

        draw_status_message();
    }
}

void TerminalPanel::enter_prompt(const String& message) {
    printf("\033[%d;1H", rows());
    fputs("\033[0J", stdout);
    printf("%.*s", cols(), message.string());
    fflush(stdout);

    for (;;) {
        KeyPress press = read_key();
        if (press.key == KeyPress::Key::Enter) {
            return;
        }

        if (press.key == KeyPress::Backspace) {
            if (m_prompt_buffer.is_empty()) {
                continue;
            }

            m_prompt_buffer.remove_index(m_prompt_buffer.size() - 1);
            fputs("\033[1D \033[1D", stdout);
            fflush(stdout);
            continue;
        }

        if (isascii(press.key)) {
            m_prompt_buffer += String(press.key);
            write(STDOUT_FILENO, &press.key, 1);
        }
    }
}

String TerminalPanel::prompt(const String& prompt) {
    enter_prompt(prompt);
    flush();
    return move(m_prompt_buffer);
}
