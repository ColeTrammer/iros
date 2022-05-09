#include <app/layout_engine.h>
#include <assert.h>
#include <clipboard/connection.h>
#include <edit/document.h>
#include <edit/document_type.h>
#include <edit/keyboard_action.h>
#include <edit/line_renderer.h>
#include <edit/text_range_collection.h>
#include <eventloop/event.h>
#include <ext/path.h>
#include <liim/utf8_view.h>
#include <stdlib.h>
#include <tinput/terminal_glyph.h>
#include <tinput/terminal_renderer.h>
#include <tui/application.h>

#include "terminal_display.h"
#include "terminal_prompt.h"
#include "terminal_search.h"
#include "terminal_status_bar.h"

static int s_display_count = 0;

TerminalDisplay::TerminalDisplay() {
    s_display_count++;
}

void TerminalDisplay::did_attach() {
    set_accepts_focus(true);
    set_min_layout_constraint({ 20, 10 });

    set_key_bindings(Edit::get_key_bindings(base()));

    on<App::KeyDownEvent>([this](const App::KeyDownEvent& event) {
        if (event.key() == App::Key::Escape) {
            hide_prompt_panel();
            hide_search_panel();
        }
        return false;
    });

    on<App::ResizeEvent>([this](const App::ResizeEvent&) {
        if (m_prompt_panel) {
            m_prompt_panel->set_positioned_rect(positioned_rect().with_height(3));
        }

        if (m_search_panel) {
            auto width = min(sized_rect().width(), 30);
            m_search_panel->set_positioned_rect(
                { positioned_rect().x() + (sized_rect().width() - width), positioned_rect().y(), width, 4 });
        }
    });

    on<App::FocusedEvent>([this](const App::FocusedEvent&) {
        if (document() && !document()->input_text_mode()) {
            TerminalStatusBar::the().set_active_display(&base());
        }
    });

    Panel::did_attach();
}

TerminalDisplay::~TerminalDisplay() {
    if (--s_display_count == 0) {
        TUI::Application::the().main_event_loop().set_should_exit(true);
    }
}

void TerminalDisplay::document_did_change() {
    if (document()) {
        compute_cols_needed_for_line_numbers();
    }
}

App::ObjectBoundCoroutine TerminalDisplay::quit() {
    if (document() && document()->modified() && !document()->input_text_mode()) {
        auto result = co_await prompt("Quit without saving? ");
        if (!result.has_value() || (result.value() != "y" && result.value() != "yes")) {
            co_return;
        }
    }

    auto* parent = this->parent();
    remove();

    // Give focus to another display, namely, the first one we can find.
    if (parent) {
        if (auto first_child = parent->children().get_or(0, nullptr); first_child && first_child->is_base_widget()) {
            static_cast<App::Widget&>(*first_child).make_focused();
        }
    }
}

void TerminalDisplay::install_document_listeners(Edit::Document& document) {
    listen<Edit::AddLines, Edit::DeleteLines, Edit::MergeLines, Edit::SplitLines>(document, [this](auto&) {
        compute_cols_needed_for_line_numbers();
    });
}

void TerminalDisplay::did_set_show_line_numbers() {
    compute_cols_needed_for_line_numbers();
}

void TerminalDisplay::set_cols_needed_for_line_numbers(int value) {
    if (m_cols_needed_for_line_numbers == value) {
        return;
    }

    m_cols_needed_for_line_numbers = value;
    invalidate_all_lines();
}

void TerminalDisplay::compute_cols_needed_for_line_numbers() {
    if (auto* doc = document()) {
        if (show_line_numbers()) {
            int num_lines = doc->line_count();
            if (num_lines == 0 || doc->input_text_mode()) {
                set_cols_needed_for_line_numbers(0);
                return;
            }

            int digits = 0;
            while (num_lines > 0) {
                num_lines /= 10;
                digits++;
            }

            set_cols_needed_for_line_numbers(digits + 1);
            return;
        }
    }
    set_cols_needed_for_line_numbers(0);
}

int TerminalDisplay::cols() const {
    return sized_rect().width() - m_cols_needed_for_line_numbers;
}

void TerminalDisplay::send_status_message(String message) {
    TerminalStatusBar::the().set_status_message(move(message));
}

Option<Point> TerminalDisplay::cursor_position() {
    if (!document()) {
        return {};
    }

    auto position = display_position_of_index(main_cursor().index());
    if (position.row() < 0 || position.row() >= rows() || position.col() < 0 || position.col() >= cols()) {
        return {};
    }
    return Point { position.col() + m_cols_needed_for_line_numbers, position.row() };
}

void TerminalDisplay::render() {
    if (!document()) {
        return;
    }

    m_last_rendered_row = 0;
    render_lines();

    auto empty_rows = rows() - m_last_rendered_row - 1;
    auto renderer = get_renderer();
    renderer.clear_rect({ 0, rows() - empty_rows, sized_rect().width(), empty_rows });

    Panel::render();
}

Edit::TextIndex TerminalDisplay::text_index_at_mouse_position(const Point& point) {
    return text_index_at_display_position({ point.y(), point.x() - m_cols_needed_for_line_numbers });
}

void TerminalDisplay::output_line(int row, int col_offset, const Edit::RenderedLine& line, int line_index) {
    m_last_rendered_row = row;

    auto renderer = get_renderer();

    if (show_line_numbers()) {
        auto line_number_rect = Rect { 0, row, m_cols_needed_for_line_numbers, 1 };
        auto line_start_index = text_index_at_display_position({ row, -col_offset });
        if (line_start_index.index_into_line() == 0) {
            auto line_number_text = String::format("%*d ", m_cols_needed_for_line_numbers - 1, line_start_index.line_index() + 1);
            renderer.render_text(line_number_rect, line_number_text.view());
        } else {
            renderer.clear_rect(line_number_rect);
        }
    }

    auto visible_line_rect = Rect { m_cols_needed_for_line_numbers, row, sized_rect().width() - m_cols_needed_for_line_numbers, 1 };
    renderer.set_clip_rect(visible_line_rect);

    auto text_width = line.position_ranges()[line_index].last().end.col();

    auto text_rect = visible_line_rect.translated({ -col_offset, 0 }).with_width(text_width);
    for (auto& range : line.position_ranges()[line_index]) {
        auto rendering_info = rendering_info_for_metadata(range.metadata);
        auto style = TInput::TerminalTextStyle {
            .foreground = rendering_info.fg,
            .background = rendering_info.bg,
            .bold = rendering_info.bold,
            .invert = rendering_info.secondary_cursor,
        };

        auto glyph = TInput::TerminalGlyph { line.rendered_lines()[line_index].substring(range.byte_offset_in_rendered_string,
                                                                                         range.byte_count_in_rendered_string),
                                             range.end.col() - range.start.col() };
        renderer.put_glyph(text_rect.top_left().translated(range.start.col(), 0), glyph, style);
    }

    auto clear_rect = Rect { text_rect.right(), row, max(visible_line_rect.right() - text_rect.right(), 0), 1 };
    renderer.clear_rect(clear_rect);
}

Edit::RenderedLine TerminalDisplay::compose_line(const Edit::Line& line) {
    if (!document()) {
        return {};
    }

    auto renderer = Edit::LineRenderer { cols(), word_wrap_enabled() };

    auto glyphs = TInput::convert_to_glyphs(line.contents().view());
    auto index_into_line = 0;

    auto maybe_insert_auto_complete_text = [&] {
        if (cursors().should_show_auto_complete_text_at(*document(), line, index_into_line)) {
            auto maybe_suggestion_text = cursors().preview_auto_complete_text();
            if (maybe_suggestion_text) {
                renderer.begin_segment(index_into_line, Edit::CharacterMetadata::Flags::AutoCompletePreview,
                                       Edit::PositionRangeType::InlineAfterCursor);
                renderer.add_to_segment(maybe_suggestion_text->view(), maybe_suggestion_text->size());
                renderer.end_segment();
            }
        }
    };

    for (auto& glyph : glyphs) {
        maybe_insert_auto_complete_text();

        renderer.begin_segment(index_into_line, 0, Edit::PositionRangeType::Normal);
        if (glyph.text() == "\t") {
            auto spaces = String::repeat(' ', Edit::tab_width - (renderer.absolute_col_position() % Edit::tab_width));
            renderer.add_to_segment(spaces.view(), spaces.size());
        } else {
            renderer.add_to_segment(glyph.text().view(), glyph.width());
        }
        renderer.end_segment();

        index_into_line += glyph.text().size();
    }
    maybe_insert_auto_complete_text();

    return renderer.finish(line, 0);
}

void TerminalDisplay::invalidate_all_line_rects() {
    invalidate();
    TerminalStatusBar::the().invalidate();
}
void TerminalDisplay::invalidate_line_rect(int row_in_display) {
    invalidate(sized_rect().with_y(row_in_display).with_height(1));
    TerminalStatusBar::the().invalidate();
}

App::ObjectBoundCoroutine TerminalDisplay::do_open_prompt() {
    if (document() && document()->input_text_mode()) {
        co_return;
    }

    auto initial_value = [&]() -> String {
        if (!document()) {
            return "";
        }

        return format("{}/", Ext::Path::resolve(".")
                                 .map([](auto p) {
                                     return p.to_string();
                                 })
                                 .value_or("."));
    }();

    auto result = co_await prompt("Open: ", move(initial_value));
    if (!result) {
        co_return;
    }

    auto document_or_error = Edit::Document::create_from_file(*result);
    if (document_or_error.is_error()) {
        send_status_message(format("Failed to open file `{}': {}", *result, strerror(document_or_error.error())));
        co_return;
    }

    set_document(move(document_or_error.value()));
}

int TerminalDisplay::enter() {
    make_focused();
    return 0;
}

void TerminalDisplay::hide_prompt_panel() {
    if (!m_prompt_panel) {
        return;
    }

    m_prompt_panel->remove();
    m_prompt_panel = nullptr;
}

void TerminalDisplay::hide_search_panel() {
    if (!m_search_panel) {
        return;
    }

    m_search_panel->remove();
    m_search_panel = nullptr;
}

Task<Option<String>> TerminalDisplay::prompt(String message, String initial_value) {
    if (m_prompt_panel) {
        hide_prompt_panel();
    }

    m_prompt_panel = create_widget_owned<TerminalPrompt>(*this, move(message), move(initial_value));
    m_prompt_panel->set_positioned_rect(positioned_rect().with_height(3));

    auto result = co_await m_prompt_panel->block_until_result(base());
    hide_prompt_panel();
    make_focused();
    co_return result;
}

void TerminalDisplay::enter_search(String initial_text) {
    if (m_search_panel) {
        m_search_panel->make_focused();
        return;
    }

    m_search_panel = create_widget_owned<TerminalSearch>(*this, move(initial_text));
    auto width = min(sized_rect().width(), 30);
    m_search_panel->set_positioned_rect({ positioned_rect().x() + (sized_rect().width() - width), positioned_rect().y(), width, 4 });
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
