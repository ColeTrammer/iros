#include <assert.h>
#include <clipboard/connection.h>
#include <edit/document.h>
#include <edit/document_type.h>
#include <edit/line_renderer.h>
#include <edit/position.h>
#include <errno.h>
#include <eventloop/event.h>
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

    s_main_panel->set_coordinates(sz.ws_row, sz.ws_col);
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
    m_cols = cols;

    if (m_absolute_row_position != -1) {
        int old_row_position = m_absolute_row_position;
        if (old_row_position >= m_rows) {
            m_absolute_row_position = max(0, m_rows - document()->num_lines());
        }

        printf("\033[%d;%dH", m_absolute_row_position + 1, 1);
        m_visible_cursor_row = 0;
        m_visible_cursor_col = 0;
    }

    if (auto* doc = document()) {
        doc->notify_panel_size_changed();
    }
}

ReplPanel::~ReplPanel() {
    restore_termios();
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

String ReplPanel::string_for_metadata(Edit::CharacterMetadata metadata) const {
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

    if (info.secondary_cursor) {
        ret += ";7";
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
                cursors().remove_secondary_cursors();
                document()->move_cursor_to_document_end(*this, cursors().main_cursor());
                document()->set_preview_auto_complete(false);
                flush();
                printf("\r\n");
                fflush(stdout);
                quit();
                return;
            }

            document()->insert_line(Edit::Line(""), document()->num_lines());
            cursors().remove_secondary_cursors();
            document()->move_cursor_to_document_end(*this, cursors().main_cursor());
            document()->scroll_cursor_into_view(*this, cursors().main_cursor());
            flush_if_needed();
        };

        notify_line_count_changed();
        schedule_update();
    }
}

void ReplPanel::quit() {
    m_should_exit = true;
    m_exit_code = 1;
}

int ReplPanel::index(int row, int col) const {
    return row * cols() + col;
}

static int string_print_width(const StringView& string) {
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

Edit::RenderedLine ReplPanel::compose_line(const Edit::Line& line) const {
    if (!document()) {
        return {};
    }

    auto renderer = Edit::LineRenderer { cols(), document()->word_wrap_enabled() };
    auto& prompt = &line == &document()->first_line() ? m_main_prompt : m_secondary_prompt;
    renderer.begin_segment(0, 0, Edit::PositionRangeType::InlineBeforeCursor);
    renderer.add_to_segment(prompt.view(), string_print_width(prompt.view()));
    renderer.end_segment();

    for (int index_into_line = 0; index_into_line <= line.length(); index_into_line++) {
        if (cursors().should_show_auto_complete_text_at(*document(), line, index_into_line)) {
            auto maybe_suggestion_text = cursors().preview_auto_complete_text(*this);
            if (maybe_suggestion_text) {
                renderer.begin_segment(index_into_line, Edit::CharacterMetadata::Flags::AutoCompletePreview,
                                       Edit::PositionRangeType::InlineAfterCursor);
                renderer.add_to_segment(maybe_suggestion_text->view(), maybe_suggestion_text->size());
                renderer.end_segment();
            }
        }

        if (index_into_line == line.length()) {
            break;
        }

        renderer.begin_segment(index_into_line, 0, Edit::PositionRangeType::Normal);
        char c = line.char_at(index_into_line);
        if (c == '\t') {
            auto spaces = String::repeat(' ', Edit::tab_width - (renderer.absolute_col_position() % Edit::tab_width));
            renderer.add_to_segment(spaces.view(), spaces.size());
        } else {
            renderer.add_to_segment(StringView { &c, &c }, 1);
        }
        renderer.end_segment();
    }
    return renderer.finish(line);
}

void ReplPanel::draw_cursor() {
    auto cursor_pos = document()->cursor_position_on_panel(*this, cursors().main_cursor());
    auto cursor_row = cursor_pos.row;
    auto cursor_col = cursor_pos.col;

    if (cursor_row >= 0 && cursor_row < rows() && cursor_col >= 0 && cursor_col < cols()) {
        if (cursor_row < m_visible_cursor_row) {
            printf("\033[%dA", m_visible_cursor_row - cursor_row);
        } else if (cursor_row > m_visible_cursor_row) {
            printf("\033[%dB", cursor_row - m_visible_cursor_row);
        }

        if (cursor_col < m_visible_cursor_col) {
            printf("\033[%dD", m_visible_cursor_col - cursor_col);
        } else if (cursor_col > m_visible_cursor_col) {
            printf("\033[%dC", cursor_col - m_visible_cursor_col);
        }

        printf("\033[?25h");

        m_visible_cursor_row = cursor_row;
        m_visible_cursor_col = cursor_col;
    } else {
        printf("\033[?25l");
    }
}

void ReplPanel::send_status_message(String) {}

void ReplPanel::print_char(char c, Edit::CharacterMetadata metadata) {
    if (c == '\0') {
        c = ' ';
    }

    if (metadata != m_last_metadata_rendered) {
        m_last_metadata_rendered = metadata;
        fputs(string_for_metadata(metadata).string(), stdout);
    }

    fputc(c, stdout);
    m_visible_cursor_col = min(m_visible_cursor_col + 1, cols() - 1);
}

void ReplPanel::output_line(int row, int col_offset, const StringView& text, const Vector<Edit::CharacterMetadata>& metadata_vector) {
    assert(col_offset == 0);

    for (size_t i = 0; i < text.size(); i++) {
        auto metadata = metadata_vector[i];
        if (metadata != m_last_metadata_rendered) {
            m_last_metadata_rendered = metadata;
            fputs(string_for_metadata(metadata).string(), stdout);
        }
        fputc(text[i], stdout);
    }

    printf("\033[0K");

    if (scroll_row_offset() + row == document()->num_rendered_lines(*this) - 1) {
        m_visible_cursor_row = row;
        m_visible_cursor_col = string_print_width(text);
    } else {
        printf("\r\n");
        m_visible_cursor_row = row + 1;
        m_visible_cursor_col = 0;
    }
}

void ReplPanel::do_open_prompt() {}

void ReplPanel::flush() {
    if (m_should_exit) {
        return;
    }

    m_render_scheduled = false;

    fputs("\033[?25l\r", stdout);
    m_visible_cursor_col = 0;

    if (m_visible_cursor_row > 0) {
        printf("\033[%dA", m_visible_cursor_row);
        m_visible_cursor_row = 0;
    }

    document()->display(*this);

    printf("\033[0J");
    draw_cursor();
    fflush(stdout);
}

void ReplPanel::flush_if_needed() {
    if (!m_render_scheduled) {
        return;
    }
    flush();
}

Vector<Variant<App::KeyEvent, App::MouseEvent>> ReplPanel::read_input() {
    using K = App::KeyEvent;
    using M = App::MouseEvent;
    using T = Variant<K, M>;
    using R = Vector<T>;

    auto key_from_character = [&](char c) -> App::Key {
        if (isascii(c)) {
            return static_cast<App::Key>(c - 'A' + 1);
        }
        return App::Key::None;
    };

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
            return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::Escape, 0 } });
        }

        if (escape_buffer[0] != '[' && escape_buffer[0] != 'O') {
            return R::create_from_single_element(
                T { K { App::KeyEventType::Down, "", key_from_character(toupper(escape_buffer[0])), App::KeyModifier::Alt } });
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
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::UpArrow, modifiers } });
                    case 'B':
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::DownArrow, modifiers } });
                    case 'C':
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::RightArrow, modifiers } });
                    case 'D':
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::LeftArrow, modifiers } });
                    case 'F':
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::End, modifiers } });
                    case 'H':
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::Home, modifiers } });
                    case 'P':
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F1, modifiers } });
                    case 'Q':
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F2, modifiers } });
                    case 'R':
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F3, modifiers } });
                    case 'S':
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F4, modifiers } });
                    default:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::None, modifiers } });
                }
            };

            auto vt_sequence_to_key = [](int num, int modifiers) -> R {
                switch (num) {
                    case 1:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::Home, modifiers } });
                    case 2:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::Insert, modifiers } });
                    case 3:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::Delete, modifiers } });
                    case 4:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::End, modifiers } });
                    case 5:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::PageUp, modifiers } });
                    case 6:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::PageDown, modifiers } });
                    case 7:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::Home, modifiers } });
                    case 8:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::End, modifiers } });
                    case 10:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F1, modifiers } });
                    case 12:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F2, modifiers } });
                    case 13:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F3, modifiers } });
                    case 14:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F4, modifiers } });
                    case 15:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F5, modifiers } });
                    case 17:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F6, modifiers } });
                    case 18:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F7, modifiers } });
                    case 19:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F8, modifiers } });
                    case 20:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F9, modifiers } });
                    case 21:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F10, modifiers } });
                    case 23:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F11, modifiers } });
                    case 24:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F12, modifiers } });
                    case 25:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F13, modifiers } });
                    case 26:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F14, modifiers } });
                    case 28:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F15, modifiers } });
                    case 29:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F16, modifiers } });
                    case 31:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F17, modifiers } });
                    case 32:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F18, modifiers } });
                    case 33:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F19, modifiers } });
                    case 34:
                        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::F20, modifiers } });
                    default:
                        return {};
                }
            };

            ret = read(STDIN_FILENO, escape_buffer + 1, 1);
            assert(ret >= 0);

            if (ret == 0) {
                return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::Escape, App::KeyModifier::Alt } });
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

                auto events = m_mouse_press_tracker.notify_mouse_event(buttons_down, cx - 1, cy - 1, z, 0);

                R out_events;
                for (auto& event : events) {
                    auto text_index = document()->text_index_at_scrolled_position(*this, { cy - m_absolute_row_position - 1, cx - 1 });
                    event->set_y(text_index.line_index());
                    event->set_x(text_index.index_into_line());
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
        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::Backspace, 0 } });
    }

    if (ch == '\r') {
        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::Enter, 0 } });
    }

    if (ch == '\t') {
        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::Tab, 0 } });
    }

    if (ch == ('w' & 0x1F)) {
        // control backspace unfortunately binds to control w, but control backspace
        // takes prcedence.
        return R::create_from_single_element(T { K { App::KeyEventType::Down, "", App::Key::Backspace, App::KeyModifier::Control } });
    }

    if (ch >= ('a' & 0x1F) && ch <= ('z' & 0x1F)) {
        return R::create_from_single_element(
            T { K { App::KeyEventType::Down, "", key_from_character(ch | 0b1000000), App::KeyModifier::Control } });
    }

    return R::create_from_single_element(T { K { App::KeyEventType::Down, String(ch), App::Key::None, 0 } });
}

Edit::Suggestions ReplPanel::get_suggestions() const {
    auto content_string = document()->content_string();
    auto cursor_index = document()->cursor_index_in_content_string(cursors().main_cursor());
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
    return Edit::Suggestions { suggestions_object.suggestion_offset(), move(new_suggestions) };
}

void ReplPanel::handle_suggestions(const Edit::Suggestions& suggestions) {
    if (++m_consecutive_tabs >= 2) {
        auto cursor_row_max = min(rows(), document()->num_rendered_lines(*this)) - 1;
        if (m_visible_cursor_row < cursor_row_max) {
            printf("\033[%dB", cursor_row_max - m_visible_cursor_row);
        }
        printf("\r\n");
        for (auto& suggestion : suggestions.suggestion_list()) {
            printf("%s ", suggestion.string());
        }
        printf("\r\n");
        fflush(stdout);

        m_visible_cursor_row = 0;
        m_visible_cursor_col = 0;

        flush();
    }
}

Vector<SharedPtr<Edit::Document>>& ReplPanel::ensure_history_documents() {
    if (m_history_documents.empty()) {
        m_history_documents.resize(m_repl.history().size() + 1);
    }
    return m_history_documents;
}

void ReplPanel::put_history_document(SharedPtr<Edit::Document> document, int index) {
    ensure_history_documents()[index] = move(document);
}

SharedPtr<Edit::Document> ReplPanel::history_document(int index) {
    auto& documents = ensure_history_documents();
    if (documents[index]) {
        return move(documents[index]);
    }

    auto& new_document_text = m_repl.history().item(index);
    return Edit::Document::create_from_text(new_document_text);
}

void ReplPanel::move_history_up() {
    if (m_history_index == 0) {
        return;
    }

    auto current_document = document_as_shared();
    auto new_document = history_document(m_history_index - 1);
    new_document->copy_settings_from(*current_document);
    put_history_document(move(current_document), m_history_index);

    set_document(new_document);
    new_document->move_cursor_to_document_end(*this, cursors().main_cursor());
    new_document->scroll_cursor_into_view(*this, cursors().main_cursor());
    flush();

    m_history_index--;
}

void ReplPanel::move_history_down() {
    if (m_history_index == m_repl.history().size()) {
        return;
    }

    auto current_document = document_as_shared();
    auto new_document = history_document(m_history_index + 1);
    new_document->copy_settings_from(*current_document);
    put_history_document(move(current_document), m_history_index);

    set_document(new_document);
    new_document->move_cursor_to_document_end(*this, cursors().main_cursor());
    new_document->scroll_cursor_into_view(*this, cursors().main_cursor());
    flush();

    m_history_index++;
}

void ReplPanel::get_absolute_row_position() {
    m_absolute_row_position = -1;

    printf("\033[6n");
    fflush(stdout);

    char buffer[65];
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDOUT_FILENO, &set);

    if (select(2, &set, nullptr, nullptr, nullptr) < 0) {
        return;
    }

    if (read(STDOUT_FILENO, buffer, sizeof(buffer) - 1) < 0) {
        return;
    }

    int row;
    int col;
    if (sscanf(buffer, "\033[%d;%dR", &row, &col) < 2) {
        return;
    }

    m_absolute_row_position = row - 1;
}

int ReplPanel::enter() {
    get_absolute_row_position();

    fd_set set;
    for (;;) {
        flush_if_needed();

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
                if (ev.is<App::KeyEvent>()) {
                    auto key_event = ev.as<App::KeyEvent>();
                    if (key_event.key() == App::Key::C && key_event.modifiers() == App::KeyModifier::Control) {
                        document->set_preview_auto_complete(false);
                        flush();
                        printf("^C\r\n");
                        fflush(stdout);
                        set_quit_by_interrupt();
                        quit();
                        break;
                    }

                    if (key_event.key() == App::Key::D && key_event.modifiers() == App::KeyModifier::Control) {
                        if (document->num_lines() == 1 && document->content_string().is_empty()) {
                            set_quit_by_eof();
                            quit();
                            break;
                        }
                        continue;
                    }

                    if (key_event.key() == App::Key::UpArrow && cursors().main_cursor().line_index() == 0) {
                        move_history_up();
                        break;
                    }

                    if (key_event.key() == App::Key::DownArrow && cursors().main_cursor().line_index() == document->num_lines() - 1) {
                        move_history_down();
                        break;
                    }

                    if (key_event.key() != App::Key::Tab) {
                        m_consecutive_tabs = 0;
                    }

                    document->notify_key_pressed(*this, ev.as<App::KeyEvent>());
                } else {
                    document->notify_mouse_event(*this, ev.as<App::MouseEvent>());
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
