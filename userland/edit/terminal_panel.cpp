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
#include <unistd.h>

#include "terminal_panel.h"

static termios s_original_termios;
static bool s_raw_mode_enabled;

static TerminalPanel* s_main_panel;
static TerminalPanel* s_prompt_panel;
static String s_prompt_message;

static void restore_termios() {
    tcsetattr(STDOUT_FILENO, TCSAFLUSH, &s_original_termios);

    fputs("\033[?1049l\033[?2004l\033[?1002l\033[?1006l\033[?25h", stdout);
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

    fputs("\033[?1049h\033[?2004h\033[?1002h\033[?1006h", stdout);
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

    if (auto* doc = document()) {
        doc->notify_panel_size_changed();
    }
}

TerminalPanel::~TerminalPanel() {}

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

    if (info.secondary_cursor) {
        ret += ";7";
    }

    ret += "m";
    return ret;
}

void TerminalPanel::document_did_change() {
    if (document()) {
        notify_line_count_changed();
        schedule_update();
    }
}

void TerminalPanel::quit() {
    m_should_exit = true;
    m_exit_code = 1;
}

void TerminalPanel::notify_line_count_changed() {
    Panel::notify_line_count_changed();

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
    auto cursor_pos = document()->cursor_position_on_panel(*this, cursors().main_cursor());
    auto cursor_row = cursor_pos.row;
    auto cursor_col = cursor_pos.col;
    if (cursor_row >= 0 && cursor_row < rows() && cursor_col >= 0 && cursor_col < cols()) {
        printf("\033[%d;%dH\033[?25h", m_row_offset + cursor_row + 1, m_col_offset + cursor_col + m_cols_needed_for_line_numbers + 1);
    } else {
        printf("\033[?25l");
    }
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

    auto cursor_col = cursors()
                          .main_cursor()
                          .referenced_line(*document())
                          .absoulte_col_offset_of_index(*document(), *this, cursors().main_cursor().index_into_line());
    auto position_string = String::format("%d,%d", cursors().main_cursor().line_index() + 1, cursor_col + 1);
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

void TerminalPanel::output_line(int row, int col_offset, const StringView& text, const Vector<Edit::CharacterMetadata>& metadata) {
    printf("\033[%d;%dH", m_row_offset + row + 1, m_col_offset + 1);

    if (document()->show_line_numbers()) {
        auto line_number_text = String::repeat(' ', m_cols_needed_for_line_numbers);
        auto line_start_index = document()->text_index_at_scrolled_position(*this, { row, 0 });
        if (line_start_index.index_into_line() == 0) {
            line_number_text = String::format("%*d ", m_cols_needed_for_line_numbers - 1, line_start_index.line_index() + 1);
        }
        printf("%s", line_number_text.string());
    }

    auto cols = TerminalPanel::cols();
    for (size_t c = col_offset; c < static_cast<size_t>(cols + col_offset) && c < text.size(); c++) {
        print_char(text[c], metadata[c]);
    }

    // Reset modifiers after every render, so that status bar, tty clear command, etc. are unaffected.
    m_last_metadata_rendered = Edit::CharacterMetadata();
    fputs(string_for_metadata(m_last_metadata_rendered).string(), stdout);

    printf("\033[K");
}

Edit::RenderedLine TerminalPanel::compose_line(const Edit::Line& line) const {
    assert(document());
    auto renderer = Edit::LineRenderer { cols(), document()->word_wrap_enabled() };
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
    m_render_scheduled = false;

    fputs("\033[?25l", stdout);

    document()->display(*this);

    printf("\033[0J");

    draw_status_message();
    draw_cursor();
    fflush(stdout);
}

void TerminalPanel::flush_if_needed() {
    if (!m_render_scheduled) {
        return;
    }
    flush();
}

int TerminalPanel::enter() {
    fd_set set;
    for (;;) {
        flush_if_needed();

        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        ssize_t ret = select(STDIN_FILENO + 1, &set, nullptr, nullptr, nullptr);
        if (ret == -1 && errno == EINTR) {
            continue;
        }

        assert(ret >= 0);
        if (ret == 0) {
            continue;
        }

        uint8_t buffer[4096];
        ret = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (ret < 0) {
            if (ret == EINTR) {
                continue;
            }
            assert(false);
        }

        m_input_parser.stream_data({ buffer, static_cast<size_t>(ret) });
        auto input = m_input_parser.take_events();
        if (auto* document = Panel::document()) {
            for (auto& ev : input) {
                if (ev->type() == App::Event::Type::Key) {
                    document->notify_key_pressed(*this, static_cast<const App::KeyEvent&>(*ev));
                } else if (ev->type() == App::Event::Type::Mouse) {
                    auto event_copy = static_cast<const App::MouseEvent&>(*ev);
                    auto text_index = document->text_index_at_scrolled_position(
                        *this, { event_copy.y() - m_row_offset, event_copy.x() - m_col_offset - m_cols_needed_for_line_numbers });
                    event_copy.set_y(text_index.line_index());
                    event_copy.set_x(text_index.index_into_line());
                    document->notify_mouse_event(*this, event_copy);
                }
            }
        }

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

    auto document = Edit::Document::create_single_line(move(staring_text));
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

    TerminalPanel text_panel(1, m_col_offset + m_cols - message_size, rows() + m_row_offset, message_size);
    s_prompt_panel = &text_panel;
    s_prompt_message = message;

    auto document = Edit::Document::create_single_line(move(starting_text));
    text_panel.set_document(document);
    document->select_all(text_panel, text_panel.cursors().main_cursor());

    m_show_status_bar = false;

    fd_set set;
    for (;;) {
        printf("\033[%d;%dH", m_row_offset + m_rows + 1, m_col_offset + 1);
        printf("%.*s", m_cols / 2, message.string());
        text_panel.flush();

        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        ssize_t ret = select(STDIN_FILENO + 1, &set, nullptr, nullptr, nullptr);
        if (ret == -1 && errno == EINTR) {
            continue;
        }

        assert(ret >= 0);
        if (ret == 0) {
            continue;
        }

        uint8_t buffer[4096];
        ret = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (ret < 0) {
            if (ret == EINTR) {
                continue;
            }
            assert(false);
        }

        m_input_parser.stream_data({ buffer, static_cast<size_t>(ret) });
        auto input = m_input_parser.take_events();
        if (auto* document = Panel::document()) {
            for (auto& ev : input) {
                if (ev->type() == App::Event::Type::Key) {
                    auto& event = static_cast<const App::KeyEvent&>(*ev);
                    if (event.key() == App::Key::Enter) {
                        cursors().remove_secondary_cursors();
                        TerminalPanel::document()->move_cursor_to_next_search_match(*this, cursors().main_cursor());
                    }

                    if (event.key() == App::Key::Escape ||
                        ((event.modifiers() & App::KeyModifier::Control) && event.key() == App::Key::Q)) {
                        goto exit_search;
                    }

                    document->notify_key_pressed(*this, event);
                } else if (ev->type() == App::Event::Type::Mouse) {
                    auto event_copy = static_cast<const App::MouseEvent&>(*ev);
                    auto text_index = document->text_index_at_scrolled_position(
                        *this, { event_copy.y() - m_row_offset, event_copy.x() - m_col_offset - m_cols_needed_for_line_numbers });
                    event_copy.set_y(text_index.line_index());
                    event_copy.set_x(text_index.index_into_line());
                    document->notify_mouse_event(*this, event_copy);
                }
            }
        }

        auto search_text = text_panel.document()->content_string();
        TerminalPanel::document()->set_search_text(search_text);
        flush_if_needed();
    }

exit_search:
    m_show_status_bar = true;

    printf("\033[%d;%dH", m_row_offset + m_rows + 1, m_col_offset + 1);
    fputs("\033[0K", stdout);
    draw_cursor();
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
