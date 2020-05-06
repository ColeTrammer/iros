#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "editor.h"
#include "terminal_panel.h"

static termios s_original_termios;
static bool s_raw_mode_enabled;

static TerminalPanel* s_main_panel;
static TerminalPanel* s_prompt_panel;
static String s_prompt_message;

static void restore_termios() {
    tcsetattr(STDOUT_FILENO, TCSAFLUSH, &s_original_termios);

#ifndef __os_2__
    fputs("\033[?1049l", stdout);
#else
    fputs("\033[1;1H\033[2J", stdout);
#endif /* __os_2__ */
    fflush(stdout);
}

static void update_panel_sizes() {
    assert(s_main_panel);

    fputs("\033[2J", stdout);
    fflush(stdout);

    winsize sz;
    assert(ioctl(STDOUT_FILENO, TIOCGWINSZ, &sz) == 0);
    s_main_panel->set_coordinates(0, 0, sz.ws_row - 1, sz.ws_col);

    if (s_prompt_panel) {
        printf("\033[%d;%dH", sz.ws_row + 1, 1);
        fputs("\033[0K", stdout);

        int message_size = LIIM::min(s_prompt_message.size(), sz.ws_col / 2);
        printf("%.*s", sz.ws_col / 2, s_prompt_message.string());
        fflush(stdout);

        s_prompt_panel->set_coordinates(sz.ws_row - 1, message_size, 1, sz.ws_col - message_size);
    }
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

    signal(SIGWINCH, [](int) {
        update_panel_sizes();
    });

    atexit(restore_termios);

    setvbuf(stdout, nullptr, _IOFBF, BUFSIZ);

#ifndef __os_2__
    fputs("\033[?1049h", stdout);
#endif /* __os_2__ */
}

constexpr int status_bar_height = 1;
constexpr time_t status_message_timeout = 3;

TerminalPanel::TerminalPanel() {
    assert(isatty(STDOUT_FILENO));

    if (!s_raw_mode_enabled) {
        enable_raw_mode();
    }

    s_main_panel = this;
    update_panel_sizes();
}

TerminalPanel::TerminalPanel(int rows, int cols, int row_off, int col_off) {
    set_coordinates(row_off, col_off, rows, cols);
}

void TerminalPanel::set_coordinates(int row_off, int col_off, int rows, int cols) {
    m_rows = rows;
    m_cols = cols;
    m_row_offset = row_off;
    m_col_offset = col_off;
    m_chars.resize(m_rows * m_cols);

    if (auto* doc = document()) {
        doc->notify_panel_size_changed();
    }
}

TerminalPanel::~TerminalPanel() {}

void TerminalPanel::clear() {
    draw_cursor();
    memset(m_chars.vector(), 0, m_chars.size());
}

void TerminalPanel::draw_cursor() {
    printf("\033[%d;%dH", m_row_offset + m_cursor_row + 1, m_col_offset + m_cursor_col + 1);
    fflush(stdout);
}

void TerminalPanel::draw_status_message() {
    if (m_status_message.is_empty()) {
        return;
    }

    fputs("\033[s", stdout);
    printf("\033[%d;%dH", m_row_offset + m_rows + 1, m_col_offset + 1);
    fputs("\033[0K", stdout);

    if (time(nullptr) - m_status_message_time > status_message_timeout) {
        m_status_message = "";
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
    fputs("\033[?25l", stdout);
    for (int r = 0; r < rows(); r++) {
        printf("\033[%d;%dH\033[0K", m_row_offset + r + 1, m_col_offset + 1);
        flush_row(r);
    }
    fputs("\033[?25h", stdout);
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

    if (ch == '\t') {
        // \t is not a control key
        return { 0, '\t' };
    }

    if (ch >= ('a' & 0x1F) && ch <= ('z' & 0x1F)) {
        return { KeyPress::Modifier::Control, ch | 0b1000000 };
    }

    return { 0, ch };
}

void TerminalPanel::enter() {
    for (;;) {
        KeyPress press = read_key();
        if (press.key == KeyPress::Key::Enter && m_stop_on_enter) {
            return;
        }

        if (auto* document = Panel::document()) {
            document->notify_key_pressed(press);
        }

        draw_status_message();
    }
}

String TerminalPanel::enter_prompt(const String& message) {
    printf("\033[%d;%dH", m_row_offset + m_rows + 1, m_col_offset + 1);
    fputs("\033[0K", stdout);

    int message_size = LIIM::min(message.size(), cols() / 2);
    printf("%.*s", cols() / 2, message.string());
    fflush(stdout);

    TerminalPanel text_panel(1, m_col_offset + cols() - message_size, rows() + m_row_offset, message_size);
    text_panel.set_stop_on_enter(true);
    s_prompt_panel = &text_panel;
    s_prompt_message = message;

    auto document = Document::create_single_line(text_panel);
    text_panel.set_document(move(document));
    text_panel.enter();

    return text_panel.document()->content_string();
}

String TerminalPanel::prompt(const String& prompt) {
    String result = enter_prompt(prompt);

    printf("\033[%d;%dH", m_row_offset + m_rows + 1, m_col_offset + 1);
    fputs("\033[0K", stdout);
    draw_cursor();
    return result;
}
