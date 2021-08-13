#include <clipboard/connection.h>
#include <edit/document.h>
#include <edit/document_type.h>
#include <edit/line_renderer.h>
#include <edit/position.h>
#include <eventloop/event.h>
#include <graphics/point.h>
#include <repl/repl_base.h>
#include <stdlib.h>
#include <tinput/io_terminal.h>
#include <tinput/terminal_glyph.h>
#include <tinput/terminal_renderer.h>
#include <tui/application.h>

#include "repl_display.h"
#include "suggestions_panel.h"

namespace Repl {
ReplDisplay::ReplDisplay(ReplBase& repl) : m_repl(repl) {
    m_main_prompt = repl.get_main_prompt();
    m_secondary_prompt = repl.get_secondary_prompt();

    m_history_index = m_repl.history().size();
}

ReplDisplay::~ReplDisplay() {}

void ReplDisplay::document_did_change() {
    if (document()) {
        document()->on_submit = [this] {
            auto input_text = document()->content_string();
            auto input_status = m_repl.get_input_status(input_text);

            if (input_status == Repl::InputStatus::Finished) {
                cursors().remove_secondary_cursors();
                document()->move_cursor_to_document_end(*this, cursors().main_cursor());
                document()->set_preview_auto_complete(false);
                invalidate();
                quit();
                deferred_invoke([] {
                    printf("\r\n");
                });
                return;
            }

            document()->insert_line(Edit::Line(""), document()->num_lines());
            cursors().remove_secondary_cursors();
            document()->move_cursor_to_document_end(*this, cursors().main_cursor());
            document()->scroll_cursor_into_view(*this, cursors().main_cursor());
        };

        notify_line_count_changed();
        schedule_update();
    }
}

void ReplDisplay::quit() {
    TUI::Application::the().event_loop().set_should_exit(true);
}

int ReplDisplay::enter() {
    make_focused();
    return 0;
}

Maybe<Point> ReplDisplay::cursor_position() {
    if (!document()) {
        return {};
    }

    auto position = document()->cursor_position_on_display(*this, cursors().main_cursor());
    return Point { position.col, position.row };
}

void ReplDisplay::move_up_rows(int count) {
    auto terminal_rect = TUI::Application::the().io_terminal().terminal_rect();
    auto new_height = sized_rect().height() + count;
    set_positioned_rect({ 0, terminal_rect.height() - new_height, sized_rect().width(), new_height });

    TUI::Application::the().io_terminal().scroll_up(count);
}

void ReplDisplay::render() {
    if (!document()) {
        return;
    }

    if (positioned_rect().top() != 0 && document()->num_rendered_lines(*this) > sized_rect().height()) {
        auto terminal_rect = TUI::Application::the().io_terminal().terminal_rect();
        auto new_height = min(document()->num_rendered_lines(*this), terminal_rect.height());

        auto delta_height = new_height - sized_rect().height();
        move_up_rows(delta_height);
        scroll_up(delta_height);
    }

    document()->display(*this);

    auto empty_rows = scroll_row_offset() + rows() - document()->num_rendered_lines(*this);
    auto renderer = get_renderer();
    renderer.clear_rect({ 0, rows() - empty_rows, sized_rect().width(), empty_rows });

    return Panel::render();
}

void ReplDisplay::on_resize() {
    if (document()) {
        document()->notify_display_size_changed();
    }

    return Panel::on_resize();
}

void ReplDisplay::on_key_event(const App::KeyEvent& event) {
    if (!document()) {
        return;
    }

    if (m_suggestions_panel) {
        switch (event.key()) {
            case App::Key::Enter:
            case App::Key::Tab:
            case App::Key::UpArrow:
            case App::Key::DownArrow:
            case App::Key::Escape:
                return m_suggestions_panel->on_key_event(event);
            default:
                break;
        }
    }

    if (event.key() == App::Key::R && event.control_down()) {
        m_suggest_based_on_history = true;
        document()->notify_key_pressed(*this, App::KeyEvent { App::KeyEventType::Down, "", App::Key::Space, App::KeyModifier::Control });
        return;
    }

    if (event.key() == App::Key::C && event.control_down()) {
        deferred_invoke([] {
            printf("^C\r\n");
        });
        set_quit_by_interrupt();
        quit();
        return;
    }

    if (event.key() == App::Key::D && event.control_down()) {
        if (document()->num_lines() == 1 && document()->content_string().empty()) {
            set_quit_by_eof();
            quit();
        }
        return;
    }

    if (event.key() == App::Key::UpArrow && cursors().main_cursor().line_index() == 0) {
        move_history_up();
        return;
    }

    if (event.key() == App::Key::DownArrow && cursors().main_cursor().line_index() == document()->num_lines() - 1) {
        move_history_down();
        return;
    }

    if (event.key() != App::Key::Tab) {
        m_consecutive_tabs = 0;
    }

    document()->notify_key_pressed(*this, event);
}

void ReplDisplay::on_mouse_event(const App::MouseEvent& event) {
    if (!document()) {
        return;
    }

    if (document()->notify_mouse_event(*this, event)) {
        return;
    }

    return Panel::on_mouse_event(event);
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

Edit::RenderedLine ReplDisplay::compose_line(const Edit::Line& line) {
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
            renderer.add_to_segment(StringView { &c, 1 }, 1);
        }
        renderer.end_segment();
    }
    return renderer.finish(line);
}

void ReplDisplay::send_status_message(String) {}

void ReplDisplay::output_line(int row, int col_offset, const StringView& text, const Vector<Edit::CharacterMetadata>& metadata) {
    auto renderer = get_renderer();

    auto visible_line_rect = Rect { 0, row, sized_rect().width(), 1 };
    renderer.set_clip_rect(visible_line_rect);

    auto text_width = TInput::convert_to_glyphs(text).total_width();

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

void ReplDisplay::do_open_prompt() {}

void ReplDisplay::suggestions_did_change(const Maybe<Edit::TextRange>& old_text_range) {
    if (m_suggestions_panel) {
        if (m_suggest_based_on_history) {
            m_suggestions_panel->did_update_suggestions();
            return;
        }

        auto& current_text_range = suggestions().current_text_range();
        if ((current_text_range.has_value() && !old_text_range.has_value()) ||
            (!current_text_range.has_value() && old_text_range.has_value())) {
            hide_suggestions_panel();
            return;
        }
        if (!current_text_range && !old_text_range) {
            m_suggestions_panel->did_update_suggestions();
            return;
        }
        if (current_text_range->start() != old_text_range->start()) {
            hide_suggestions_panel();
            return;
        }
        m_suggestions_panel->did_update_suggestions();
    }
}

void ReplDisplay::do_compute_suggestions() {
    if (m_suggest_based_on_history) {
        auto& history = m_repl.history();
        auto suggestions = Vector<Edit::Suggestion> {};
        for (int i = history.size() - 1; i >= 0; i--) {
            auto& item = m_repl.history().item(i);
            suggestions.add({ item, {} });
        }
        return set_suggestions(move(suggestions));
    }

    return set_suggestions(m_repl.get_suggestions(*document(), cursors().main_cursor().index()));
}

void ReplDisplay::show_suggestions_panel() {
    if (m_suggestions_panel) {
        return;
    }

    auto cursor_position = document()->cursor_position_on_display(*this, cursors().main_cursor());

    m_suggestions_panel = add<SuggestionsPanel>(*this).shared_from_this();

    auto suggestions_rect = Rect { positioned_rect().x(), positioned_rect().y() + cursor_position.row + 1, sized_rect().width(),
                                   m_suggestions_panel->layout_constraint().height() };
    if (suggestions_rect.bottom() > positioned_rect().bottom()) {
        auto ideal_rows_to_move_up = suggestions_rect.bottom() - positioned_rect().bottom();
        auto rows_to_move_up = min(positioned_rect().top(), ideal_rows_to_move_up);
        if (rows_to_move_up != 0) {
            move_up_rows(rows_to_move_up);
            suggestions_rect.set_y(suggestions_rect.y() - rows_to_move_up);
        }
    }

    m_suggestions_panel->set_positioned_rect(suggestions_rect);
}

void ReplDisplay::hide_suggestions_panel() {
    if (!m_suggestions_panel) {
        return;
    }

    m_suggest_based_on_history = false;

    TUI::Application::the().invalidate(m_suggestions_panel->positioned_rect());
    remove_child(m_suggestions_panel);
    m_suggestions_panel = nullptr;

    make_focused();
}

void ReplDisplay::complete_suggestion(const Edit::MatchedSuggestion& suggestion) {
    document()->insert_suggestion(*this, suggestion);
    if (m_suggest_based_on_history) {
        hide_suggestions_panel();
    }
}

Vector<SharedPtr<Edit::Document>>& ReplDisplay::ensure_history_documents() {
    if (m_history_documents.empty()) {
        m_history_documents.resize(m_repl.history().size() + 1);
    }
    return m_history_documents;
}

void ReplDisplay::put_history_document(SharedPtr<Edit::Document> document, int index) {
    ensure_history_documents()[index] = move(document);
}

SharedPtr<Edit::Document> ReplDisplay::history_document(int index) {
    auto& documents = ensure_history_documents();
    if (documents[index]) {
        return move(documents[index]);
    }

    auto& new_document_text = m_repl.history().item(index);
    return Edit::Document::create_from_text(new_document_text);
}

void ReplDisplay::move_history_up() {
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
    new_document->invalidate_rendered_contents(cursors().main_cursor().referenced_line(*new_document));
    invalidate();

    m_history_index--;
}

void ReplDisplay::move_history_down() {
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
    new_document->invalidate_rendered_contents(cursors().main_cursor().referenced_line(*new_document));
    invalidate();

    m_history_index++;
}

Edit::TextIndex ReplDisplay::text_index_at_mouse_position(const Point& point) {
    return document()->text_index_at_scrolled_position(*this, { point.y(), point.x() });
}

Maybe<String> ReplDisplay::enter_prompt(const String&, String) {
    return {};
}

void ReplDisplay::enter_search(String) {}

void ReplDisplay::set_clipboard_contents(String text, bool is_whole_line) {
    m_prev_clipboard_contents = move(text);
    m_prev_clipboard_contents_were_whole_line = is_whole_line;
    Clipboard::Connection::the().set_clipboard_contents_to_text(m_prev_clipboard_contents);
}

String ReplDisplay::clipboard_contents(bool& is_whole_line) const {
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
