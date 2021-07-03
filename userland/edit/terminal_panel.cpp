#include <assert.h>
#include <clipboard/connection.h>
#include <edit/document.h>
#include <edit/document_type.h>
#include <edit/key_press.h>
#include <errno.h>
#include <eventloop/event.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include "terminal_panel.h"

static termios s_original_termios;
static bool s_raw_mode_enabled;

static TerminalPanel* s_main_panel;
static TerminalPanel* s_prompt_panel;
static String s_prompt_message;

static void restore_termios() {
    tcsetattr(STDOUT_FILENO, TCSAFLUSH, &s_original_termios);

    fputs("\033[?1049l\033[?1002l\033[?1006l\033[?25h", stdout);
    fflush(stdout);
}

static void update_panel_sizes() {
    assert(s_main_panel);

    fputs("\033[2J", stdout);

    winsize sz;
    assert(ioctl(STDOUT_FILENO, TIOCGWINSZ, &sz) == 0);
    s_main_panel->set_coordinates(0, 0, sz.ws_row - 1, sz.ws_col);

    if (s_prompt_panel) {
        printf("\033[%d;%dH", sz.ws_row + 1, 1);
        fputs("\033[0K", stdout);

        int message_size = LIIM::min(s_prompt_message.size(), static_cast<size_t>(sz.ws_col / 2));
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

    fputs("\033[?1049h\033[?1002h\033[?1006h", stdout);
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
    m_dirty_rows.resize(m_rows);
    for (auto& b : m_dirty_rows) {
        b = true;
    }

    if (auto* doc = document()) {
        doc->notify_panel_size_changed();
    }
}

TerminalPanel::~TerminalPanel() {}

void TerminalPanel::clear() {
    draw_cursor();
    m_screen_info.clear();
    m_screen_info.resize(m_rows * TerminalPanel::cols());
    flush();
}

static int vga_color_to_number(vga_color color, bool background) {
    int ret = 0;

    switch (color) {
        case VGA_COLOR_BLACK:
            ret = 30;
            break;
        case VGA_COLOR_RED:
            ret = 31;
            break;
        case VGA_COLOR_GREEN:
            ret = 32;
            break;
        case VGA_COLOR_BROWN:
            ret = 33;
            break;
        case VGA_COLOR_BLUE:
            ret = 34;
            break;
        case VGA_COLOR_MAGENTA:
            ret = 35;
            break;
        case VGA_COLOR_CYAN:
            ret = 36;
            break;
        case VGA_COLOR_LIGHT_GREY:
            ret = 37;
            break;
        case VGA_COLOR_DARK_GREY:
            ret = 90;
            break;
        case VGA_COLOR_LIGHT_RED:
            ret = 91;
            break;
        case VGA_COLOR_LIGHT_GREEN:
            ret = 92;
            break;
        case VGA_COLOR_YELLOW:
            ret = 93;
            break;
        case VGA_COLOR_LIGHT_BLUE:
            ret = 94;
            break;
        case VGA_COLOR_LIGHT_MAGENTA:
            ret = 95;
            break;
        case VGA_COLOR_LIGHT_CYAN:
            ret = 96;
            break;
        case VGA_COLOR_WHITE:
            ret = 97;
            break;
    }

    return background ? ret + 10 : ret;
}

String TerminalPanel::string_for_metadata(Edit::CharacterMetadata metadata) const {
    String ret = "\033[0";

    RenderingInfo info = rendering_info_for_metadata(metadata);
    if (info.bold) {
        ret += ";1";
    }

    if (info.fg.has_value()) {
        ret += String::format(";%d", vga_color_to_number(info.fg.value(), false));
    } else {
        ret += ";39";
    }

    if (info.bg.has_value()) {
        ret += String::format(";%d", vga_color_to_number(info.bg.value(), true));
    } else {
        ret += ";49";
    }

    ret += "m";
    return ret;
}

void TerminalPanel::document_did_change() {
    if (document()) {
        clear();
        notify_line_count_changed();
        document()->display();
    }
}

void TerminalPanel::quit() {
    m_should_exit = true;
    m_exit_code = 1;
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
            if (num_lines == 0 || doc->input_text_mode()) {
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
    auto cursor_row = document()->cursor_row_on_panel();
    auto cursor_col = document()->cursor_col_on_panel();
    if (cursor_row >= 0 && cursor_row < rows() && cursor_col >= 0 && cursor_col < cols()) {
        printf("\033[%d;%dH\033[?25h", m_row_offset + cursor_row + 1, m_col_offset + cursor_col + m_cols_needed_for_line_numbers + 1);
    } else {
        printf("\033[?25l");
    }
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

    auto cursor_row_position = document()->cursor_row_on_panel() + document()->row_offset();
    auto cursor_col_position = document()->cursor_col_on_panel() + document()->col_offset();

    auto position_string = String::format("%d,%d", cursor_row_position + 1, cursor_col_position + 1);
    auto status_rhs = String::format("%s%s [%s] %9s", name.string(), document()->modified() ? "*" : " ",
                                     document_type_to_string(document()->type()).string(), position_string.string());

    size_t width_for_message = m_cols - status_rhs.size() - 4;
    String fill_chars = "    ";
    if (m_status_message.size() > width_for_message) {
        fill_chars = "... ";
    }
    printf("%-*.*s%s%s", static_cast<int>(width_for_message), static_cast<int>(width_for_message), m_status_message.string(),
           fill_chars.string(), status_rhs.string());

    fputs("\033[u", stdout);
}

void TerminalPanel::send_status_message(String message) {
    m_status_message = move(message);
    m_status_message_time = time(nullptr);
    draw_status_message();
}

void TerminalPanel::set_text_at(int row, int col, char c, Edit::CharacterMetadata metadata) {
    auto& info = m_screen_info[index(row, col)];
    if (info.ch != c || info.metadata != metadata) {
        m_screen_info[index(row, col)] = { c, metadata };
        m_dirty_rows[row] = true;
    }
}

void TerminalPanel::print_char(char c, Edit::CharacterMetadata metadata) {
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

    if (row < rows() - 1) {
        fputc('\r', stdout);
        fputc('\n', stdout);
    }
}

void TerminalPanel::do_open_prompt() {
    auto result = prompt("Open: ");
    if (!result.has_value()) {
        return;
    }

    auto document = Edit::Document::create_from_file(result.value().string(), *this);
    if (!document) {
        send_status_message(String::format("Failed to open `%s'", result.value().string()));
        return;
    }

    set_document(move(document));
}

void TerminalPanel::flush() {
    fputs("\033[?25l", stdout);

    for (int r = 0; r < rows(); r++) {
        if (!m_dirty_rows[r]) {
            continue;
        }
        m_dirty_rows[r] = false;

        printf("\033[%d;%dH\033[0K", m_row_offset + r + 1, m_col_offset + 1);

        int line_number = document()->row_offset() + r + 1;
        if (document()->show_line_numbers() && line_number <= document()->num_lines()) {
            m_last_metadata_rendered = Edit::CharacterMetadata();
            fputs(string_for_metadata(m_last_metadata_rendered).string(), stdout);

            char buf[48];
            snprintf(buf, sizeof(buf) - 1, "%*d ", m_cols_needed_for_line_numbers - 1, line_number);
            fputs(buf, stdout);
        }
        flush_row(r);
    }

    // Reset modifiers after every render, so that status bar, etc. are unaffected.
    m_last_metadata_rendered = Edit::CharacterMetadata();
    fputs(string_for_metadata(m_last_metadata_rendered).string(), stdout);

    draw_status_message();
    draw_cursor();
    fflush(stdout);
}

Vector<Variant<Edit::KeyPress, App::MouseEvent>> TerminalPanel::read_input() {
    using K = Edit::KeyPress;
    using M = App::MouseEvent;
    using T = Variant<K, M>;
    using R = Vector<T>;

    char ch;
    ssize_t ret = read(STDIN_FILENO, &ch, 1);
    assert(ret >= 0);
    if (ret == 0) {
        return {};
    }

    if (ch == '\033') {
        char escape_buffer[64] = { 0 };
        ret = read(STDIN_FILENO, escape_buffer, 1);
        assert(ret >= 0);

        if (ret == 0) {
            return R::create_from_single_element(T { K { 0, Edit::KeyPress::Key::Escape } });
        }

        if (escape_buffer[0] != '[' && escape_buffer[0] != 'O') {
            return R::create_from_single_element(T { K { Edit::KeyPress::Modifier::Alt, toupper(escape_buffer[0]) } });
        } else {
            // Information from https://en.wikipedia.org/wiki/ANSI_escape_code#Terminal_input_sequences
            auto modifiers_from_digit = [](char digit) -> int {
                if (!isdigit(digit) || digit < '0' || digit > '8') {
                    return 0;
                }
                return digit - 1;
            };

            auto xterm_sequence_to_key = [](char ch, int modifiers) -> R {
                switch (ch) {
                    case 'A':
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::UpArrow } });
                    case 'B':
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::DownArrow } });
                    case 'C':
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::RightArrow } });
                    case 'D':
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::LeftArrow } });
                    case 'F':
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::End } });
                    case 'H':
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::Home } });
                    case 'P':
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F1 } });
                    case 'Q':
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F2 } });
                    case 'R':
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F3 } });
                    case 'S':
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F4 } });
                    default:
                        return R::create_from_single_element(T { K { modifiers, ch } });
                }
            };

            auto vt_sequence_to_key = [](int num, int modifiers) -> R {
                switch (num) {
                    case 1:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::Home } });
                    case 2:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::Insert } });
                    case 3:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::Delete } });
                    case 4:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::End } });
                    case 5:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::PageUp } });
                    case 6:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::PageDown } });
                    case 7:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::Home } });
                    case 8:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::End } });
                    case 10:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F0 } });
                    case 11:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F1 } });
                    case 12:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F2 } });
                    case 13:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F3 } });
                    case 14:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F4 } });
                    case 15:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F5 } });
                    case 17:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F6 } });
                    case 18:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F7 } });
                    case 19:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F8 } });
                    case 20:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F9 } });
                    case 21:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F10 } });
                    case 23:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F11 } });
                    case 24:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F12 } });
                    case 25:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F13 } });
                    case 26:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F14 } });
                    case 28:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F15 } });
                    case 29:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F16 } });
                    case 31:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F17 } });
                    case 32:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F18 } });
                    case 33:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F19 } });
                    case 34:
                        return R::create_from_single_element(T { K { modifiers, Edit::KeyPress::Key::F20 } });
                    default:
                        return {};
                }
            };

            ret = read(STDIN_FILENO, escape_buffer + 1, 1);
            assert(ret >= 0);

            if (ret == 0) {
                return R::create_from_single_element(T { K { Edit::KeyPress::Modifier::Alt, Edit::KeyPress::Key::Escape } });
            }

            if (escape_buffer[1] == '<') {
                // SGR encoded mouse events (enabled with DECSET 1006)
                // Information from https://github.com/chromium/hterm/blob/master/doc/ControlSequences.md#sgr
                size_t escape_buffer_length = 2;
                for (;;) {
                    ret = read(STDIN_FILENO, &escape_buffer[escape_buffer_length++], 1);
                    if (escape_buffer[escape_buffer_length - 1] == 'M' || escape_buffer[escape_buffer_length - 1] == 'm') {
                        break;
                    }
                    if (ret <= 0) {
                        return {};
                    }
                }

                int cb;
                int cx;
                int cy;
                if (sscanf(escape_buffer, "[<%d;%d;%d", &cb, &cx, &cy) != 3) {
                    return {};
                }

                bool mouse_down = escape_buffer[escape_buffer_length - 1] == 'M';
                int z = 0;

                int buttons_down = m_mouse_press_tracker.prev_buttons();
                switch (cb & ~0b11100 /* ignore modifiers for now */) {
                    case 0:
                        // Left mouse button
                        if (mouse_down) {
                            buttons_down |= App::MouseButton::Left;
                        } else {
                            buttons_down &= ~App::MouseButton::Left;
                        }
                        break;
                    case 1:
                        // Middle mouse button (ignored for now)
                        break;
                    case 2:
                        // Right mouse button
                        if (mouse_down) {
                            buttons_down |= App::MouseButton::Right;
                        } else {
                            buttons_down &= ~App::MouseButton::Right;
                        }
                        break;
                    case 32:
                    case 33:
                    case 34:
                        // Mouse move.
                        break;
                    case 64:
                        // Scroll up
                        z = -1;
                        break;
                    case 65:
                        // Scroll down
                        z = 1;
                        break;
                }

                auto events = m_mouse_press_tracker.notify_mouse_event(buttons_down, cx - 1, cy - 1, z);

                R out_events;
                for (auto& event : events) {
                    event->set_y(clamp(document()->index_of_line_at_position(cy - m_row_offset - 1), 0, document()->num_lines() - 1));
                    event->set_x(document()->index_into_line(event->y(), cx - m_col_offset - m_cols_needed_for_line_numbers - 1));
                    out_events.add(*event);
                }
                return out_events;
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

    if (ch == s_original_termios.c_cc[VERASE]) {
        return R::create_from_single_element(T { K { 0, Edit::KeyPress::Key::Backspace } });
    }

    if (ch == '\r') {
        return R::create_from_single_element(T { K { 0, Edit::KeyPress::Key::Enter } });
    }

    if (ch == '\t') {
        // \t is not a control key
        return R::create_from_single_element(T { K { 0, '\t' } });
    }

    if (ch == ('w' & 0x1F)) {
        // control backspace unfortunately binds to control w, but control backspace
        // takes prcedence.
        return R::create_from_single_element(T { K { Edit::KeyPress::Modifier::Control, Edit::KeyPress::Key::Backspace } });
    }

    if (ch >= ('a' & 0x1F) && ch <= ('z' & 0x1F)) {
        return R::create_from_single_element(T { K { Edit::KeyPress::Modifier::Control, ch | 0b1000000 } });
    }

    return R::create_from_single_element(T { K { 0, ch } });
}

int TerminalPanel::enter() {
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

        auto input = read_input();
        if (input.empty()) {
            continue;
        }

        if (auto* document = Panel::document()) {
            for (auto& ev : input) {
                if (ev.is<Edit::KeyPress>()) {
                    document->notify_key_pressed(ev.as<Edit::KeyPress>());
                } else {
                    document->notify_mouse_event(ev.as<App::MouseEvent>());
                }
            }
        }

        draw_status_message();

        if (m_should_exit) {
            break;
        }
    }

    return m_exit_code;
}

Maybe<String> TerminalPanel::enter_prompt(const String& message, String staring_text) {
    printf("\033[%d;%dH", m_row_offset + m_rows + 1, m_col_offset + 1);
    fputs("\033[0K", stdout);

    int message_size = LIIM::min(message.size(), static_cast<size_t>(m_cols / 2));
    printf("%.*s", m_cols / 2, message.string());
    fflush(stdout);

    TerminalPanel text_panel(1, m_col_offset + m_cols - message_size, rows() + m_row_offset, message_size);
    s_prompt_panel = &text_panel;
    s_prompt_message = message;

    auto document = Edit::Document::create_single_line(text_panel, move(staring_text));
    document->on_submit = [&] {
        text_panel.m_exit_code = 0;
        text_panel.m_should_exit = true;
    };
    text_panel.set_document(move(document));

    if (text_panel.enter() != 0) {
        return {};
    }

    return text_panel.document()->content_string();
}

Maybe<String> TerminalPanel::prompt(const String& prompt) {
    Maybe<String> result = enter_prompt(prompt);

    printf("\033[%d;%dH", m_row_offset + m_rows + 1, m_col_offset + 1);
    fputs("\033[0K", stdout);
    draw_cursor();
    fflush(stdout);
    return result;
}

void TerminalPanel::enter_search(String starting_text) {
    printf("\033[%d;%dH", m_row_offset + m_rows + 1, m_col_offset + 1);
    fputs("\033[0K", stdout);

    String message = "Find: ";
    int message_size = LIIM::min(message.size(), static_cast<size_t>(m_cols / 2));
    printf("%.*s", m_cols / 2, message.string());
    fflush(stdout);

    TerminalPanel text_panel(1, m_col_offset + m_cols - message_size, rows() + m_row_offset, message_size);
    s_prompt_panel = &text_panel;
    s_prompt_message = message;

    auto document = Edit::Document::create_single_line(text_panel, move(starting_text));
    text_panel.set_document(move(document));

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

        auto input = text_panel.read_input();
        if (input.empty()) {
            continue;
        }

        for (auto& ev : input) {
            if (ev.is<Edit::KeyPress>()) {
                auto& press = ev.as<Edit::KeyPress>();
                if (press.key == Edit::KeyPress::Key::Enter) {
                    TerminalPanel::document()->move_cursor_to_next_search_match();
                }

                if (press.key == Edit::KeyPress::Key::Escape ||
                    ((press.modifiers & Edit::KeyPress::Modifier::Control) && press.key == 'Q')) {
                    goto exit_search;
                }

                text_panel.document()->notify_key_pressed(press);
            } else {
                text_panel.document()->notify_mouse_event(ev.as<App::MouseEvent>());
            }
        }

        auto search_text = text_panel.document()->content_string();
        TerminalPanel::document()->set_search_text(search_text);
        TerminalPanel::document()->display_if_needed();

        text_panel.draw_cursor();
        fflush(stdout);
    }

exit_search:
    m_show_status_bar = true;

    printf("\033[%d;%dH", m_row_offset + m_rows + 1, m_col_offset + 1);
    fputs("\033[0K", stdout);
    draw_cursor();
    fflush(stdout);
}

void TerminalPanel::notify_now_is_a_good_time_to_draw_cursor() {
    draw_cursor();
    draw_status_message();
    fflush(stdout);
}

void TerminalPanel::set_clipboard_contents(String text, bool is_whole_line) {
    m_prev_clipboard_contents = move(text);
    m_prev_clipboard_contents_were_whole_line = is_whole_line;
    Clipboard::Connection::the().set_clipboard_contents_to_text(m_prev_clipboard_contents);
}

String TerminalPanel::clipboard_contents(bool& is_whole_line) const {
    auto contents = Clipboard::Connection::the().get_clipboard_contents_as_text();
    if (!contents.has_value()) {
        is_whole_line = m_prev_clipboard_contents_were_whole_line;
        return m_prev_clipboard_contents;
    }

    auto& ret = contents.value();
    if (ret == m_prev_clipboard_contents) {
        is_whole_line = m_prev_clipboard_contents_were_whole_line;
    } else {
        m_prev_clipboard_contents = "";
        is_whole_line = m_prev_clipboard_contents_were_whole_line = false;
    }
    return move(ret);
}
