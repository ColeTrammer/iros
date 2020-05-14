#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include "document.h"
#include "document_type.h"
#include "key_press.h"
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

    // 100 ms timeout on reads
    to_set.c_cc[VMIN] = 0;
    to_set.c_cc[VTIME] = 1;

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
    m_screen_info.resize(m_rows * TerminalPanel::cols());

    if (auto* doc = document()) {
        doc->notify_panel_size_changed();
    }
}

TerminalPanel::~TerminalPanel() {}

void TerminalPanel::clear() {
    draw_cursor();
    m_screen_info.clear();
    m_screen_info.resize(m_rows * m_cols);
}

const String& TerminalPanel::string_for_metadata(CharacterMetadata metadata) const {
    static String default_string("\033[0m");
    static String highlight_string("\033[0;30;103m");
    static String selected_string("\033[0;107;30m");
    static String operator_string("\033[0;36m");
    static String keyword_string("\033[0;1;32m");
    static String number_string("\033[0;31m");
    static String identifier_string("\033[0;1;93m");

    if (metadata.selected()) {
        return selected_string;
    }

    if (metadata.highlighted()) {
        return highlight_string;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxOperator) {
        return operator_string;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxKeyword) {
        return keyword_string;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxNumber) {
        return number_string;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxIdentifier) {
        return identifier_string;
    }

    return default_string;
}

void TerminalPanel::document_did_change() {
    if (document()) {
        notify_line_count_changed();
    }
}

void TerminalPanel::notify_line_count_changed() {
    int old_cols_needed_for_line_numbers = m_cols_needed_for_line_numbers;
    compute_cols_needed_for_line_numbers();

    if (old_cols_needed_for_line_numbers != m_cols_needed_for_line_numbers) {
        // Updates character storage and notifies the document
        set_coordinates(m_row_offset, m_col_offset, m_rows, m_cols);
    }
}

void TerminalPanel::compute_cols_needed_for_line_numbers() {
    if (auto* doc = document()) {
        if (doc->show_line_numbers()) {
            int num_lines = doc->num_lines();
            if (num_lines == 0 || doc->single_line_mode()) {
                m_cols_needed_for_line_numbers = 0;
                return;
            }

            int digits = 0;
            while (num_lines > 0) {
                num_lines /= 10;
                digits++;
            }

            m_cols_needed_for_line_numbers = digits + 1;
            return;
        }
    }
    m_cols_needed_for_line_numbers = 0;
}

int TerminalPanel::cols() const {
    return m_cols - m_cols_needed_for_line_numbers;
}

void TerminalPanel::draw_cursor() {
    printf("\033[%d;%dH", m_row_offset + m_cursor_row + 1, m_col_offset + m_cursor_col + m_cols_needed_for_line_numbers + 1);
    fflush(stdout);
}

void TerminalPanel::draw_status_message() {
    if (this != s_main_panel || !document() || !m_show_status_bar) {
        return;
    }

    fputs("\033[s", stdout);
    printf("\033[%d;%dH", m_row_offset + m_rows + 1, m_col_offset + 1);
    fputs("\033[0K", stdout);

    if (time(nullptr) - m_status_message_time > status_message_timeout) {
        m_status_message = "";
    }

    auto& name = document()->name().is_empty() ? String("[Unamed File]") : document()->name();
    auto position_string = String::format("%d,%d", document()->cursor_row_position() + 1, document()->cursor_col_position() + 1);
    auto status_rhs = String::format("%s%s [%s] %9s", name.string(), document()->modified() ? "*" : " ",
                                     document_type_to_string(document()->type()).string(), position_string.string());

    int width_for_message = m_cols - status_rhs.size() - 4;
    String fill_chars = "    ";
    if (m_status_message.size() > width_for_message) {
        fill_chars = "... ";
    }
    printf("%-*.*s%s%s", width_for_message, width_for_message, m_status_message.string(), fill_chars.string(), status_rhs.string());

    fputs("\033[u", stdout);
    fflush(stdout);
}

void TerminalPanel::send_status_message(String message) {
    m_status_message = move(message);
    m_status_message_time = time(nullptr);
    draw_status_message();
}

void TerminalPanel::set_text_at(int row, int col, char c, CharacterMetadata metadata) {
    m_screen_info[index(row, col)] = { c, metadata };
}

void TerminalPanel::set_cursor(int row, int col) {
    if (m_cursor_row == row && m_cursor_col == col) {
        return;
    }

    m_cursor_row = row;
    m_cursor_col = col;
    draw_cursor();
}

void TerminalPanel::print_char(char c, CharacterMetadata metadata) {
    if (c == '\0') {
        c = ' ';
    }

    if (metadata != m_last_metadata_rendered) {
        m_last_metadata_rendered = metadata;
        fputs(string_for_metadata(metadata).string(), stdout);
    }

    fputc(c, stdout);
}

void TerminalPanel::flush_row(int row) {
    int cols = TerminalPanel::cols();
    for (int c = 0; c < cols; c++) {
        auto& info = m_screen_info[index(row, c)];
        print_char(info.ch, info.metadata);
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

        int line_number = document()->row_offset() + r + 1;
        if (document()->show_line_numbers() && line_number <= document()->num_lines()) {
            char buf[48];
            snprintf(buf, sizeof(buf) - 1, "%*d ", m_cols_needed_for_line_numbers - 1, line_number);
            fputs(buf, stdout);
        }
        flush_row(r);
    }

    // Reset modifiers after every render, so that status bar, etc. are unaffected.
    m_last_metadata_rendered = CharacterMetadata();
    fputs(string_for_metadata(m_last_metadata_rendered).string(), stdout);

    fputs("\033[?25h", stdout);
    draw_status_message();
    draw_cursor();
}

Maybe<KeyPress> TerminalPanel::read_key() {
    char ch;
    ssize_t ret = read(STDIN_FILENO, &ch, 1);
    assert(ret >= 0);
    if (ret == 0) {
        return {};
    }

    if (ch == '\033') {
        char escape_buffer[40];
        ret = read(STDIN_FILENO, escape_buffer, 1);
        assert(ret >= 0);

        if (ret == 0) {
            return { { 0, KeyPress::Key::Escape } };
        }

        if (escape_buffer[0] != '[' && escape_buffer[0] != 'O') {
            return { { KeyPress::Modifier::Alt, escape_buffer[0] } };
        } else {
            // Information from https://en.wikipedia.org/wiki/ANSI_escape_code#Terminal_input_sequences
            auto modifiers_from_digit = [](char digit) -> int {
                if (!isdigit(digit) || digit < '0' || digit > '8') {
                    return 0;
                }
                return digit - 1;
            };

            auto xterm_sequence_to_key = [](char ch, int modifiers) -> KeyPress {
                switch (ch) {
                    case 'A':
                        return { modifiers, KeyPress::Key::UpArrow };
                    case 'B':
                        return { modifiers, KeyPress::Key::DownArrow };
                    case 'C':
                        return { modifiers, KeyPress::Key::RightArrow };
                    case 'D':
                        return { modifiers, KeyPress::Key::LeftArrow };
                    case 'F':
                        return { modifiers, KeyPress::Key::End };
                    case 'H':
                        return { modifiers, KeyPress::Key::Home };
                    case 'P':
                        return { modifiers, KeyPress::Key::F1 };
                    case 'Q':
                        return { modifiers, KeyPress::Key::F2 };
                    case 'R':
                        return { modifiers, KeyPress::Key::F3 };
                    case 'S':
                        return { modifiers, KeyPress::Key::F4 };
                    default:
                        return { modifiers, ch };
                }
            };

            auto vt_sequence_to_key = [](int num, int modifiers) -> Maybe<KeyPress> {
                switch (num) {
                    case 1:
                        return { { modifiers, KeyPress::Key::Home } };
                    case 2:
                        return { { modifiers, KeyPress::Key::Insert } };
                    case 3:
                        return { { modifiers, KeyPress::Key::Delete } };
                    case 4:
                        return { { modifiers, KeyPress::Key::End } };
                    case 5:
                        return { { modifiers, KeyPress::Key::PageUp } };
                    case 6:
                        return { { modifiers, KeyPress::Key::PageDown } };
                    case 7:
                        return { { modifiers, KeyPress::Key::Home } };
                    case 8:
                        return { { modifiers, KeyPress::Key::End } };
                    case 10:
                        return { { modifiers, KeyPress::Key::F0 } };
                    case 11:
                        return { { modifiers, KeyPress::Key::F1 } };
                    case 12:
                        return { { modifiers, KeyPress::Key::F2 } };
                    case 13:
                        return { { modifiers, KeyPress::Key::F3 } };
                    case 14:
                        return { { modifiers, KeyPress::Key::F4 } };
                    case 15:
                        return { { modifiers, KeyPress::Key::F5 } };
                    case 17:
                        return { { modifiers, KeyPress::Key::F6 } };
                    case 18:
                        return { { modifiers, KeyPress::Key::F7 } };
                    case 19:
                        return { { modifiers, KeyPress::Key::F8 } };
                    case 20:
                        return { { modifiers, KeyPress::Key::F9 } };
                    case 21:
                        return { { modifiers, KeyPress::Key::F10 } };
                    case 23:
                        return { { modifiers, KeyPress::Key::F11 } };
                    case 24:
                        return { { modifiers, KeyPress::Key::F12 } };
                    case 25:
                        return { { modifiers, KeyPress::Key::F13 } };
                    case 26:
                        return { { modifiers, KeyPress::Key::F14 } };
                    case 28:
                        return { { modifiers, KeyPress::Key::F15 } };
                    case 29:
                        return { { modifiers, KeyPress::Key::F16 } };
                    case 31:
                        return { { modifiers, KeyPress::Key::F17 } };
                    case 32:
                        return { { modifiers, KeyPress::Key::F18 } };
                    case 33:
                        return { { modifiers, KeyPress::Key::F19 } };
                    case 34:
                        return { { modifiers, KeyPress::Key::F20 } };
                    default:
                        return {};
                }
            };

            ret = read(STDIN_FILENO, escape_buffer + 1, 1);
            assert(ret >= 0);

            if (ret == 0) {
                return { { KeyPress::Modifier::Alt, KeyPress::Key::Escape } };
            }

            if (isalpha(escape_buffer[1])) {
                return xterm_sequence_to_key(escape_buffer[1], 0);
            }

            if (isdigit(escape_buffer[1])) {
                ret = read(STDIN_FILENO, escape_buffer + 2, 1);
                assert(ret >= 0);
                if (ret == 0) {
                    ch = escape_buffer[1];
                } else {
                    if (isalpha(escape_buffer[2])) {
                        int modifiers = modifiers_from_digit(escape_buffer[1]);
                        return xterm_sequence_to_key(escape_buffer[2], modifiers);
                    }

                    size_t i = 2;
                    while (i < sizeof(escape_buffer) && (escape_buffer[i] != '~' && !isalpha(escape_buffer[i]))) {
                        ret = read(STDIN_FILENO, escape_buffer + ++i, 1);
                        assert(ret >= 0);
                        if (ret == 0) {
                            return {};
                        }
                    }

                    if (escape_buffer[i] != '~' && !isalpha(escape_buffer[i])) {
                        return {};
                    }

                    if (strchr(escape_buffer, ';')) {
                        int num;
                        char modifiers;
                        if (sscanf(escape_buffer, "[%d;%c~", &num, &modifiers) != 2) {
                            return {};
                        }

                        if (isalpha(escape_buffer[i]) && num == 1) {
                            return xterm_sequence_to_key(escape_buffer[i], modifiers_from_digit(modifiers));
                        } else if (escape_buffer[i] == '~') {
                            return vt_sequence_to_key(num, modifiers_from_digit(modifiers));
                        }
                        return {};
                    }

                    int num;
                    if (sscanf(escape_buffer, "[%d~", &num) != 1) {
                        return {};
                    }

                    return vt_sequence_to_key(num, 0);
                }
            }

            ch = escape_buffer[1];
        }
    }

    if (ch == 127) {
        return { { 0, KeyPress::Key::Backspace } };
    }

    if (ch == '\r') {
        return { { 0, KeyPress::Key::Enter } };
    }

    if (ch == '\t') {
        // \t is not a control key
        return { { 0, '\t' } };
    }

    if (ch == ('w' & 0x1F)) {
        // control backspace unfortunately binds to control w, but control backspace
        // takes prcedence.
        return { { KeyPress::Modifier::Control, KeyPress::Key::Backspace } };
    }

    if (ch >= ('a' & 0x1F) && ch <= ('z' & 0x1F)) {
        return { { KeyPress::Modifier::Control, ch | 0b1000000 } };
    }

    return { { 0, ch } };
}

void TerminalPanel::enter() {
    fd_set set;
    for (;;) {
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        int ret = select(STDIN_FILENO + 1, &set, nullptr, nullptr, nullptr);
        if (ret == -1 && errno == EINTR) {
            continue;
        }

        assert(ret >= 0);
        if (ret == 0) {
            continue;
        }

        auto maybe_press = read_key();
        if (!maybe_press.has_value()) {
            continue;
        }

        auto& press = maybe_press.value();
        if (press.key == KeyPress::Key::Enter && m_stop_on_enter) {
            return;
        }

        if (auto* document = Panel::document()) {
            document->notify_key_pressed(press);
        }

        draw_status_message();
    }
}

String TerminalPanel::enter_prompt(const String& message, String staring_text) {
    printf("\033[%d;%dH", m_row_offset + m_rows + 1, m_col_offset + 1);
    fputs("\033[0K", stdout);

    int message_size = LIIM::min(message.size(), m_cols / 2);
    printf("%.*s", m_cols / 2, message.string());
    fflush(stdout);

    TerminalPanel text_panel(1, m_col_offset + m_cols - message_size, rows() + m_row_offset, message_size);
    text_panel.set_stop_on_enter(true);
    s_prompt_panel = &text_panel;
    s_prompt_message = message;

    auto document = Document::create_single_line(text_panel, move(staring_text));
    text_panel.set_document(move(document));
    text_panel.document()->display();
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

void TerminalPanel::enter_search(String starting_text) {
    printf("\033[%d;%dH", m_row_offset + m_rows + 1, m_col_offset + 1);
    fputs("\033[0K", stdout);

    String message = "Find: ";
    int message_size = LIIM::min(message.size(), m_cols / 2);
    printf("%.*s", m_cols / 2, message.string());
    fflush(stdout);

    TerminalPanel text_panel(1, m_col_offset + m_cols - message_size, rows() + m_row_offset, message_size);
    s_prompt_panel = &text_panel;
    s_prompt_message = message;

    auto document = Document::create_single_line(text_panel, move(starting_text));
    text_panel.set_document(move(document));
    text_panel.document()->display();

    m_show_status_bar = false;

    fd_set set;
    for (;;) {
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        int ret = select(STDIN_FILENO + 1, &set, nullptr, nullptr, nullptr);
        if (ret == -1 && errno == EINTR) {
            continue;
        }

        assert(ret >= 0);
        if (ret == 0) {
            continue;
        }

        auto maybe_press = read_key();
        if (!maybe_press.has_value()) {
            continue;
        }

        auto& press = maybe_press.value();
        if (press.key == KeyPress::Key::Enter) {
            TerminalPanel::document()->move_cursor_to_next_search_match();
        }

        if (press.key == KeyPress::Key::Escape) {
            break;
        }

        text_panel.document()->notify_key_pressed(press);

        auto search_text = text_panel.document()->content_string();
        TerminalPanel::document()->set_search_text(search_text);
        TerminalPanel::document()->display_if_needed();

        text_panel.draw_cursor();
    }

    m_show_status_bar = true;

    printf("\033[%d;%dH", m_row_offset + m_rows + 1, m_col_offset + 1);
    fputs("\033[0K", stdout);
    draw_cursor();
}
