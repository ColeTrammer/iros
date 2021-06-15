#include <assert.h>
#include <clipboard/connection.h>
#include <edit/document.h>
#include <edit/document_type.h>
#include <edit/key_press.h>
#include <edit/mouse_event.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <tinput/repl.h>
#include <unistd.h>

#include "repl_panel.h"

namespace TInput {

static termios s_original_termios;
static bool s_raw_mode_enabled;

static ReplPanel* s_main_panel;

static void restore_termios() {
    tcsetattr(STDOUT_FILENO, TCSAFLUSH, &s_original_termios);

    fputs("\033[?25h", stdout);
    fflush(stdout);

    setvbuf(stdout, nullptr, _IOLBF, BUFSIZ);

    s_raw_mode_enabled = false;
}

static void update_panel_sizes() {
    assert(s_main_panel);

    winsize sz;
    assert(ioctl(STDOUT_FILENO, TIOCGWINSZ, &sz) == 0);
    s_main_panel->set_coordinates(1, sz.ws_col);
}

static void enable_raw_mode() {
    s_raw_mode_enabled = true;

    assert(tcgetattr(STDOUT_FILENO, &s_original_termios) == 0);

    termios to_set = s_original_termios;

    cfmakeraw(&to_set);

    // 100 ms timeout on reads
    to_set.c_cc[VMIN] = 0;
    to_set.c_cc[VTIME] = 1;

    assert(tcsetattr(STDOUT_FILENO, TCSAFLUSH, &to_set) == 0);

    signal(SIGWINCH, [](int) {
        update_panel_sizes();
    });

    atexit(restore_termios);

    setvbuf(stdout, nullptr, _IOFBF, BUFSIZ);
}

ReplPanel::ReplPanel(Repl& repl) : m_repl(repl) {
    assert(isatty(STDOUT_FILENO));

    m_main_prompt = repl.get_main_prompt();
    m_secondary_prompt = repl.get_secondary_prompt();

    m_history_index = m_repl.history().size();

    if (!s_raw_mode_enabled) {
        enable_raw_mode();
    }

    s_main_panel = this;
    update_panel_sizes();
}

void ReplPanel::set_coordinates(int rows, int cols) {
    m_rows = rows;
    m_max_cols = cols;
    m_screen_info.resize(cols_at_row(0) + (rows - 1) * cols_at_row(1));
    m_dirty_rows.resize(m_rows);
    for (auto& b : m_dirty_rows) {
        b = true;
    }

    if (auto* doc = document()) {
        doc->notify_panel_size_changed();
    }
}

ReplPanel::~ReplPanel() {
    restore_termios();
}

void ReplPanel::clear() {
    m_screen_info.clear();
    m_screen_info.resize(cols_at_row(0) + (m_rows - 1) * cols_at_row(1));
    for (auto& b : m_dirty_rows) {
        b = true;
    }
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

String ReplPanel::string_for_metadata(CharacterMetadata metadata) const {
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

void ReplPanel::document_did_change() {
    if (document()) {
        document()->on_submit = [this] {
            auto input_text = document()->content_string();
            auto input_status = m_repl.get_input_status(input_text);

            if (input_status == InputStatus::Finished) {
                document()->move_cursor_to_document_end();
                draw_cursor();
                printf("\r\n");
                fflush(stdout);
                quit();
                return;
            }

            set_coordinates(rows() + 1, max_cols());
            document()->insert_line(Line(""), document()->num_lines());
            document()->move_cursor_to_document_end();
        };

        m_cursor_row = 0;
        m_cursor_col = 0;

        clear();
        notify_line_count_changed();
        document()->display();
    }
}

void ReplPanel::quit() {
    m_should_exit = true;
    m_exit_code = 1;
}

int ReplPanel::index(int row, int col) const {
    if (row == 0) {
        return col;
    }

    return cols_at_row(0) + (row - 1) * cols_at_row(1) + col;
}

void ReplPanel::notify_line_count_changed() {
    if (document()->num_lines() != m_rows) {
        int new_rows = document()->num_lines();
        set_coordinates(new_rows, max_cols());
    }
}

static int string_print_width(const String& string) {
    // NOTE: naively handle TTY escape sequences as matching the regex \033(.*)[:alpha:]
    //       also UTF-8 characters are ignored.

    int count = 0;
    for (size_t i = 0; i < string.size(); i++) {
        if (string[i] != '\033') {
            count++;
            continue;
        }

        for (i = i + 1; i < string.size(); i++) {
            if (isalpha(string[i])) {
                break;
            }
        }
    }

    return count;
}

int ReplPanel::cols_at_row(int row) const {
    return max_cols() - prompt_cols_at_row(row);
}

int ReplPanel::prompt_cols_at_row(int row) const {
    return string_print_width(prompt_at_row(row));
}

const String& ReplPanel::prompt_at_row(int row) const {
    return row == 0 ? m_main_prompt : m_secondary_prompt;
}

void ReplPanel::draw_cursor() {
    int desired_cursor_col = prompt_cols_at_row(m_cursor_row) + m_cursor_col;
    if (m_cursor_row >= 0 && m_cursor_row < rows() && desired_cursor_col >= 0 && desired_cursor_col < max_cols()) {
        if (m_cursor_row < m_visible_cursor_row) {
            printf("\033[%dA", m_visible_cursor_row - m_cursor_row);
        } else if (m_cursor_row > m_visible_cursor_row) {
            printf("\033[%dB", m_cursor_row - m_visible_cursor_row);
        }

        if (desired_cursor_col < m_visible_cursor_col) {
            printf("\033[%dD", m_visible_cursor_col - desired_cursor_col);
        } else if (desired_cursor_col > m_visible_cursor_col) {
            printf("\033[%dC", desired_cursor_col - m_visible_cursor_col);
        }

        printf("\033[?25h");

        m_visible_cursor_row = m_cursor_row;
        m_visible_cursor_col = desired_cursor_col;
    } else {
        printf("\033[?25l");
    }
    fflush(stdout);
}

void ReplPanel::send_status_message(String) {}

void ReplPanel::set_text_at(int row, int col, char c, CharacterMetadata metadata) {
    auto& info = m_screen_info[index(row, col)];
    if (info.ch != c || info.metadata != metadata) {
        m_screen_info[index(row, col)] = { c, metadata };
        m_dirty_rows[row] = true;
    }
}

void ReplPanel::set_cursor(int row, int col) {
    if (m_cursor_row == row && m_cursor_col == col) {
        return;
    }

    m_cursor_row = row;
    m_cursor_col = col;
    draw_cursor();
}

void ReplPanel::print_char(char c, CharacterMetadata metadata) {
    if (c == '\0') {
        c = ' ';
    }

    if (metadata != m_last_metadata_rendered) {
        m_last_metadata_rendered = metadata;
        fputs(string_for_metadata(metadata).string(), stdout);
    }

    fputc(c, stdout);
    m_visible_cursor_col = min(m_visible_cursor_col + 1, max_cols() - 1);
}

void ReplPanel::flush_row(int row) {
    printf("%s", prompt_at_row(row).string());
    m_visible_cursor_col += prompt_cols_at_row(row);

    int cols = cols_at_row(row);
    for (int c = 0; c < cols; c++) {
        auto& info = m_screen_info[index(row, c)];
        print_char(info.ch, info.metadata);
    }

    if (row < rows() - 1) {
        fputc('\r', stdout);
        fputc('\n', stdout);

        m_visible_cursor_col = 0;
        m_visible_cursor_row++;
    }
}

void ReplPanel::do_open_prompt() {}

void ReplPanel::flush() {
    if (m_should_exit) {
        return;
    }

    fputs("\033[?25l\r", stdout);
    m_visible_cursor_col = 0;

    if (m_visible_cursor_row > 0) {
        printf("\033[%dA", m_visible_cursor_row);
        m_visible_cursor_row = 0;
    }

    for (int r = 0; r < rows(); r++) {
        if (!m_dirty_rows[r]) {
            if (r != rows() - 1) {
                printf("\r\n");
                m_visible_cursor_col = 0;
                m_visible_cursor_row++;
            } else {
                printf("\033[%dC", max_cols() - 1);
                m_visible_cursor_col = max_cols() - 1;
            }
            continue;
        }
        m_dirty_rows[r] = false;

        printf("\r\033[0K");
        m_visible_cursor_col = 0;

        flush_row(r);

        // Reset modifiers so that line prompts are unaffected.
        m_last_metadata_rendered = CharacterMetadata();
        fputs(string_for_metadata(m_last_metadata_rendered).string(), stdout);
    }

    printf("\033[0J");
    draw_cursor();
    fflush(stdout);
}

Vector<Variant<KeyPress, MouseEvent>> ReplPanel::read_input() {
    using K = KeyPress;
    using M = MouseEvent;
    using T = Variant<K, M>;
    using R = Vector<T>;

    char ch;
    ssize_t ret = read(STDIN_FILENO, &ch, 1);
    assert(ret >= 0);
    if (ret == 0) {
        return {};
    }

    if (ch == '\033') {
        char escape_buffer[64];
        ret = read(STDIN_FILENO, escape_buffer, 1);
        assert(ret >= 0);

        if (ret == 0) {
            return R::create_from_single_element(T { K { 0, KeyPress::Key::Escape } });
        }

        if (escape_buffer[0] != '[' && escape_buffer[0] != 'O') {
            return R::create_from_single_element(T { K { KeyPress::Modifier::Alt, escape_buffer[0] } });
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
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::UpArrow } });
                    case 'B':
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::DownArrow } });
                    case 'C':
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::RightArrow } });
                    case 'D':
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::LeftArrow } });
                    case 'F':
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::End } });
                    case 'H':
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::Home } });
                    case 'P':
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F1 } });
                    case 'Q':
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F2 } });
                    case 'R':
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F3 } });
                    case 'S':
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F4 } });
                    default:
                        return R::create_from_single_element(T { K { modifiers, ch } });
                }
            };

            auto vt_sequence_to_key = [](int num, int modifiers) -> R {
                switch (num) {
                    case 1:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::Home } });
                    case 2:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::Insert } });
                    case 3:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::Delete } });
                    case 4:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::End } });
                    case 5:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::PageUp } });
                    case 6:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::PageDown } });
                    case 7:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::Home } });
                    case 8:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::End } });
                    case 10:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F0 } });
                    case 11:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F1 } });
                    case 12:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F2 } });
                    case 13:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F3 } });
                    case 14:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F4 } });
                    case 15:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F5 } });
                    case 17:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F6 } });
                    case 18:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F7 } });
                    case 19:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F8 } });
                    case 20:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F9 } });
                    case 21:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F10 } });
                    case 23:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F11 } });
                    case 24:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F12 } });
                    case 25:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F13 } });
                    case 26:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F14 } });
                    case 28:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F15 } });
                    case 29:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F16 } });
                    case 31:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F17 } });
                    case 32:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F18 } });
                    case 33:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F19 } });
                    case 34:
                        return R::create_from_single_element(T { K { modifiers, KeyPress::Key::F20 } });
                    default:
                        return {};
                }
            };

            ret = read(STDIN_FILENO, escape_buffer + 1, 1);
            assert(ret >= 0);

            if (ret == 0) {
                return R::create_from_single_element(T { K { KeyPress::Modifier::Alt, KeyPress::Key::Escape } });
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
        return R::create_from_single_element(T { K { 0, KeyPress::Key::Backspace } });
    }

    if (ch == '\r') {
        return R::create_from_single_element(T { K { 0, KeyPress::Key::Enter } });
    }

    if (ch == '\t') {
        // \t is not a control key
        return R::create_from_single_element(T { K { 0, '\t' } });
    }

    if (ch == ('w' & 0x1F)) {
        // control backspace unfortunately binds to control w, but control backspace
        // takes prcedence.
        return R::create_from_single_element(T { K { KeyPress::Modifier::Control, KeyPress::Key::Backspace } });
    }

    if (ch >= ('a' & 0x1F) && ch <= ('z' & 0x1F)) {
        return R::create_from_single_element(T { K { KeyPress::Modifier::Control, ch | 0b1000000 } });
    }

    return R::create_from_single_element(T { K { 0, ch } });
}

Suggestions ReplPanel::get_suggestions() const {
    auto content_string = document()->content_string();
    auto cursor_index = document()->cursor_index_in_content_string();
    auto suggestions_object = m_repl.get_suggestions(content_string, cursor_index);

    auto& suggestions = suggestions_object.suggestion_list();
    if (suggestions_object.suggestion_count() <= 1) {
        return suggestions_object;
    }

    ::qsort(suggestions.vector(), suggestions.size(), sizeof(suggestions[0]), [](const void* p1, const void* p2) {
        const auto* s1 = reinterpret_cast<const String*>(p1);
        const auto* s2 = reinterpret_cast<const String*>(p2);
        return strcmp(s1->string(), s2->string());
    });

    size_t i;
    for (i = suggestions_object.suggestion_offset(); i < suggestions.first().size() && i < suggestions.last().size(); i++) {
        if (suggestions.first()[i] != suggestions.last()[i]) {
            break;
        }
    }

    if (i == suggestions_object.suggestion_offset()) {
        return suggestions_object;
    }

    auto new_suggestions = Vector<String>::create_from_single_element({ suggestions.first().string(), i });
    return { suggestions_object.suggestion_offset(), move(new_suggestions) };
}

void ReplPanel::handle_suggestions(const Suggestions& suggestions) {
    if (++m_consecutive_tabs >= 2) {
        if (m_visible_cursor_row < rows() - 1) {
            printf("\033[%dB", rows() - 1 - m_visible_cursor_row);
        }
        printf("\r\n");
        for (auto& suggestion : suggestions.suggestion_list()) {
            printf("%s ", suggestion.string());
        }
        printf("\r\n");
        fflush(stdout);

        m_visible_cursor_row = 0;
        m_visible_cursor_col = 0;

        clear();
        document()->display();
    }
}

Vector<UniquePtr<Document>>& ReplPanel::ensure_history_documents() {
    if (m_history_documents.empty()) {
        m_history_documents.resize(m_repl.history().size() + 1);
    }
    return m_history_documents;
}

void ReplPanel::put_history_document(UniquePtr<Document> document, int index) {
    ensure_history_documents()[index] = move(document);
}

UniquePtr<Document> ReplPanel::take_history_document(int index) {
    auto& documents = ensure_history_documents();
    if (documents[index]) {
        return move(documents[index]);
    }

    auto& new_document_text = m_repl.history().item(index);
    return Document::create_from_text(*this, new_document_text);
}

void ReplPanel::move_history_up() {
    if (m_history_index == 0) {
        return;
    }

    auto current_document = take_document();
    auto new_document = take_history_document(m_history_index - 1);
    new_document->copy_settings_from(*current_document);
    put_history_document(move(current_document), m_history_index);

    set_document(move(new_document));
    document()->move_cursor_to_document_end();

    m_history_index--;
}

void ReplPanel::move_history_down() {
    if (m_history_index == m_repl.history().size()) {
        return;
    }

    auto current_document = take_document();
    auto new_document = take_history_document(m_history_index + 1);
    new_document->copy_settings_from(*current_document);
    put_history_document(move(current_document), m_history_index);

    set_document(move(new_document));
    document()->move_cursor_to_document_end();

    m_history_index++;
}

int ReplPanel::enter() {
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
                if (ev.is<KeyPress>()) {
                    auto key_event = ev.as<KeyPress>();
                    if (key_event.key == 'C' && key_event.modifiers == KeyPress::Modifier::Control) {
                        printf("^C\r\n");
                        fflush(stdout);
                        set_quit_by_interrupt();
                        quit();
                        break;
                    }

                    if (key_event.key == 'D' && key_event.modifiers == KeyPress::Modifier::Control) {
                        if (document->num_lines() == 1 && document->content_string().is_empty()) {
                            set_quit_by_eof();
                            quit();
                            break;
                        }
                        continue;
                    }

                    if (key_event.key == KeyPress::UpArrow && document->cursor_row_position() == 0) {
                        move_history_up();
                        break;
                    }

                    if (key_event.key == KeyPress::DownArrow && document->cursor_row_position() == document->num_lines() - 1) {
                        move_history_down();
                        break;
                    }

                    if (key_event.key != '\t') {
                        m_consecutive_tabs = 0;
                    }

                    document->notify_key_pressed(ev.as<KeyPress>());
                } else {
                    document->notify_mouse_event(ev.as<MouseEvent>());
                }
            }
        }

        if (m_should_exit) {
            break;
        }
    }

    return m_exit_code;
}

Maybe<String> ReplPanel::enter_prompt(const String&, String) {
    return {};
}

Maybe<String> ReplPanel::prompt(const String&) {
    return {};
}

void ReplPanel::enter_search(String) {}

void ReplPanel::notify_now_is_a_good_time_to_draw_cursor() {
    fflush(stdout);
}

void ReplPanel::set_clipboard_contents(String text, bool is_whole_line) {
    m_prev_clipboard_contents = move(text);
    m_prev_clipboard_contents_were_whole_line = is_whole_line;
    Clipboard::Connection::the().set_clipboard_contents_to_text(m_prev_clipboard_contents);
}

String ReplPanel::clipboard_contents(bool& is_whole_line) const {
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

}
