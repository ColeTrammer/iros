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

#include "terminal_display.h"
#include "terminal_prompt.h"
#include "terminal_search.h"
#include "terminal_status_bar.h"

TerminalDisplay::TerminalDisplay() {
    set_accepts_focus(true);
}

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

void TerminalDisplay::send_status_message(String message) {
    TerminalStatusBar::the().set_status_message(move(message));
}

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

    auto empty_rows = scroll_row_offset() + rows() - document()->num_rendered_lines(*this);
    auto renderer = get_renderer();
    renderer.clear_rect({ 0, rows() - empty_rows, sized_rect().width(), empty_rows });

    Panel::render();
}

void TerminalDisplay::on_mouse_event(const App::MouseEvent& event) {
    if (!document()) {
        return;
    }

    if (document()->notify_mouse_event(*this, event)) {
        return;
    }

    Panel::on_mouse_event(event);
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

    if (m_prompt_panel) {
        m_prompt_panel->set_positioned_rect(positioned_rect().with_height(3));
    }

    if (m_search_panel) {
        auto width = min(sized_rect().width(), 20);
        m_search_panel->set_positioned_rect({ positioned_rect().x() + (sized_rect().width() - width), positioned_rect().y(), width, 3 });
    }

    return Panel::on_resize();
}

void TerminalDisplay::on_focused() {
    TerminalStatusBar::the().set_active_display(this);
}

Edit::TextIndex TerminalDisplay::text_index_at_mouse_position(const Point& point) {
    return document()->text_index_at_scrolled_position(*this, { point.y(), point.x() - m_cols_needed_for_line_numbers });
}

void TerminalDisplay::output_line(int row, int col_offset, const StringView& text, const Vector<Edit::CharacterMetadata>& metadata) {
    auto renderer = get_renderer();

    if (document()->show_line_numbers()) {
        auto line_number_rect = Rect { 0, row, m_cols_needed_for_line_numbers, 1 };
        auto line_start_index = document()->text_index_at_scrolled_position(*this, { row, 0 });
        if (line_start_index.index_into_line() == 0) {
            auto line_number_text = String::format("%*d ", m_cols_needed_for_line_numbers - 1, line_start_index.line_index() + 1);
            renderer.render_text(line_number_rect, line_number_text.view());
        } else {
            renderer.clear_rect(line_number_rect);
        }
    }

    auto visible_line_rect = Rect { m_cols_needed_for_line_numbers, row, sized_rect().width() - m_cols_needed_for_line_numbers, 1 };
    renderer.set_clip_rect(visible_line_rect);

    // FIXME: this computation is more complicated.
    auto text_width = text.size();

    auto text_rect = visible_line_rect.translated({ -col_offset, 0 }).with_width(text_width);
    renderer.render_complex_styled_text(text_rect, text, [&](size_t index) -> TInput::TerminalTextStyle {
        auto rendering_info = rendering_info_for_metadata(metadata[index]);
        return TInput::TerminalTextStyle {
            .foreground = rendering_info.fg.map([](vga_color color) {
                return Color { color };
            }),
            .background = rendering_info.bg.map([](vga_color color) {
                return Color { color };
            }),
            .bold = rendering_info.bold,
            .invert = rendering_info.secondary_cursor,
        };
    });

    auto clear_rect = Rect { text_rect.right(), row, max(visible_line_rect.right() - text_rect.right(), 0), 1 };
    renderer.clear_rect(clear_rect);
}

Edit::RenderedLine TerminalDisplay::compose_line(const Edit::Line& line) {
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
    make_focused();
    return 0;
}

void TerminalDisplay::hide_prompt_panel() {
    if (!m_prompt_panel) {
        return;
    }

    TUI::Application::the().invalidate(m_prompt_panel->positioned_rect());
    remove_child(m_prompt_panel);
    m_prompt_panel = nullptr;
}

void TerminalDisplay::prompt(String message, Function<void(Maybe<String>)> callback) {
    if (m_prompt_panel) {
        hide_prompt_panel();
    }

    m_prompt_panel = TerminalPrompt::create(shared_from_this(), move(message), "");
    m_prompt_panel->on_submit = [this, callback = move(callback)](auto result) {
        hide_prompt_panel();
        make_focused();
        callback.safe_call(move(result));
    };
    m_prompt_panel->set_positioned_rect(positioned_rect().with_height(3));
}

void TerminalDisplay::enter_search(String initial_text) {
    if (m_search_panel) {
        m_search_panel->make_focused();
        return;
    }

    m_search_panel = TerminalSearch::create(shared_from_this(), *this, move(initial_text));
    auto width = min(sized_rect().width(), 20);
    m_search_panel->set_positioned_rect({ positioned_rect().x() + (sized_rect().width() - width), positioned_rect().y(), width, 3 });
}

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
