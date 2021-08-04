#include <assert.h>
#include <clipboard/connection.h>
#include <edit/document.h>
#include <edit/document_type.h>
#include <edit/line_renderer.h>
#include <edit/position.h>
#include <eventloop/event.h>
#include <stdlib.h>
#include <tinput/terminal_renderer.h>
#include <tui/application.h>
#include <unistd.h>

#include "terminal_display.h"

TerminalDisplay::TerminalDisplay() {}

TerminalDisplay::~TerminalDisplay() {}

void TerminalDisplay::document_did_change() {
    if (document()) {
        notify_line_count_changed();
        schedule_update();
    }
}

void TerminalDisplay::quit() {
    TUI::Application::the().event_loop().set_should_exit(true);
}

void TerminalDisplay::notify_line_count_changed() {
    Display::notify_line_count_changed();

    int old_cols_needed_for_line_numbers = m_cols_needed_for_line_numbers;
    compute_cols_needed_for_line_numbers();

    if (old_cols_needed_for_line_numbers != m_cols_needed_for_line_numbers) {
        document()->notify_display_size_changed();
    }
}

void TerminalDisplay::compute_cols_needed_for_line_numbers() {
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

int TerminalDisplay::cols() const {
    return sized_rect().width() - m_cols_needed_for_line_numbers;
}

void TerminalDisplay::send_status_message(String) {}

Maybe<Point> TerminalDisplay::cursor_position() {
    if (!document()) {
        return {};
    }

    auto position = document()->cursor_position_on_display(*this, cursors().main_cursor());
    if (position.row < 0 || position.row >= rows() || position.col < 0 || position.col >= cols()) {
        return {};
    }
    return Point { position.col + m_cols_needed_for_line_numbers, position.row };
}

void TerminalDisplay::render() {
    if (!document()) {
        return;
    }

    document()->display(*this);

    auto empty_rows = m_row_offset + rows() - document()->num_rendered_lines(*this);
    auto renderer = get_renderer();
    renderer.clear_rect({ 0, rows() - empty_rows, sized_rect().width(), empty_rows });
}

void TerminalDisplay::on_mouse_event(const App::MouseEvent& event) {
    if (document()) {
        document()->notify_mouse_event(*this, event);
    }
}

void TerminalDisplay::on_key_event(const App::KeyEvent& event) {
    if (document()) {
        document()->notify_key_pressed(*this, event);
    }
}

void TerminalDisplay::on_resize() {
    if (document()) {
        document()->notify_display_size_changed();
    }
}

void TerminalDisplay::output_line(int row, int col_offset, const StringView& text, const Vector<Edit::CharacterMetadata>&) {
    auto renderer = get_renderer();
    auto visible_line_rect = Rect { m_cols_needed_for_line_numbers, row, sized_rect().width() - m_cols_needed_for_line_numbers, 1 };
    renderer.set_clip_rect(visible_line_rect);

    // FIXME: this computation is more complicated.
    auto text_width = text.size();

    auto text_rect = visible_line_rect.translated({ -col_offset, 0 }).with_width(text_width);
    renderer.render_text(text_rect, text);

    auto clear_rect = Rect { text_rect.right(), row, max(visible_line_rect.right() - text_rect.right(), 0), 1 };
    renderer.clear_rect(clear_rect);
}

Edit::RenderedLine TerminalDisplay::compose_line(const Edit::Line& line) const {
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
            renderer.add_to_segment(StringView { &c, 1 }, 1);
        }
        renderer.end_segment();
    }
    return renderer.finish(line);
}

void TerminalDisplay::do_open_prompt() {}

int TerminalDisplay::enter() {
    TUI::Application::the().set_active_panel(this);
    return 0;
}

Maybe<String> TerminalDisplay::prompt(const String&) {
    return {};
}

void TerminalDisplay::enter_search(String) {}

void TerminalDisplay::set_clipboard_contents(String text, bool is_whole_line) {
    m_prev_clipboard_contents = move(text);
    m_prev_clipboard_contents_were_whole_line = is_whole_line;
    Clipboard::Connection::the().set_clipboard_contents_to_text(m_prev_clipboard_contents);
}

String TerminalDisplay::clipboard_contents(bool& is_whole_line) const {
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
