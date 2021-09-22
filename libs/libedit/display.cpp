#include <edit/display.h>
#include <edit/document.h>
#include <edit/rendered_line.h>
#include <eventloop/widget_events.h>
#include <graphics/point.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace Edit {
Display::Display() : m_cursors { *this } {}

void Display::initialize() {
    this_widget().on_unchecked<App::ResizeEvent>({}, [this](auto&) {
        if (word_wrap_enabled()) {
            invalidate_all_lines();
            clamp_scroll_offset();
        }
    });

    this_widget().on_unchecked<App::MouseDownEvent>({}, [this](const App::MouseDownEvent& event) {
        if (!document()) {
            return false;
        }
        auto& document = *this->document();

        auto index = text_index_at_mouse_position({ event.x(), event.y() });
        if (event.left_button()) {
            document.start_input(*this, true);
            cursors().remove_secondary_cursors();
            auto& cursor = cursors().main_cursor();
            document.move_cursor_to(*this, cursor, index, MovementMode::Move);
            switch (event.cyclic_count(3)) {
                case 2:
                    document.select_word_at_cursor(*this, cursor);
                    break;
                case 3:
                    document.select_line_at_cursor(*this, cursor);
                    break;
                default:
                    break;
            }
            document.finish_input(*this, true);
            return true;
        }
        return false;
    });

    this_widget().on_unchecked<App::MouseMoveEvent>({}, [this](const App::MouseMoveEvent& event) {
        if (!document()) {
            return false;
        }
        auto& document = *this->document();

        auto index = text_index_at_mouse_position({ event.x(), event.y() });
        if (event.buttons_down() & App::MouseButton::Left) {
            document.start_input(*this, true);
            cursors().remove_secondary_cursors();
            auto& cursor = cursors().main_cursor();
            document.move_cursor_to(*this, cursor, index, MovementMode::Select);
            document.finish_input(*this, true);
            return true;
        }
        return false;
    });

    this_widget().on_unchecked<App::MouseScrollEvent>({}, [this](const App::MouseScrollEvent& event) {
        if (!document()) {
            return false;
        }
        auto& document = *this->document();

        document.start_input(*this, true);
        scroll(2 * event.z(), 0);
        document.finish_input(*this, false);
        return true;
    });

    this_widget().on_unchecked<App::TextEvent>({}, [this](const App::TextEvent& event) {
        if (!document()) {
            return false;
        }
        auto& document = *this->document();

        document.start_input(*this, true);
        document.insert_text_at_cursor(*this, event.text());
        document.finish_input(*this, true);
        return true;
    });
}

Display::~Display() {}

void Display::set_document(SharedPtr<Document> document) {
    if (m_document == document) {
        return;
    }

    if (m_document) {
        uninstall_document_listeners(*m_document);
        m_search_results.clear();
    }
    m_document = move(document);
    if (m_document) {
        install_document_listeners(*m_document);
        clear_search();
    }

    m_cursors.remove_secondary_cursors();
    m_cursors.main_cursor().reset();
    m_rendered_lines.resize(m_document->num_lines());
    invalidate_all_lines();
    set_scroll_offset({});
    hide_suggestions_panel();
    document_did_change();
}

void Display::set_suggestions(Vector<Suggestion> suggestions) {
    auto old_suggestion_range = m_suggestions.current_text_range();
    m_suggestions.set_suggestions(move(suggestions));

    auto cursor_index = cursors().main_cursor().index();
    auto index_for_suggestion = TextIndex { cursor_index.line_index(), max(cursor_index.index_into_line() - 1, 0) };
    m_suggestions.set_current_text_range(document()->syntax_highlighting_info().range_at_text_index(index_for_suggestion));

    m_suggestions.compute_matches(*document(), cursors().main_cursor());

    suggestions_did_change(old_suggestion_range);
}

void Display::compute_suggestions() {
    do_compute_suggestions();
}

void Display::clamp_scroll_offset() {
    if (!document()) {
        set_scroll_offset({});
        return;
    }

    if (scroll_offset().line_index() >= document()->num_lines()) {
        set_scroll_offset(
            { document()->last_line_index(), rendered_line_count(document()->last_line_index()) - 1, scroll_offset().relative_col() });
        return;
    }

    auto rendered_line_count = this->rendered_line_count(scroll_offset().line_index());
    if (scroll_offset().relative_row() >= rendered_line_count) {
        set_scroll_offset({ scroll_offset().line_index(), rendered_line_count - 1, 0 });
    }
}

void Display::set_scroll_offset(const AbsolutePosition& offset) {
    if (m_scroll_offset == offset) {
        return;
    }

    m_scroll_offset = offset;
    invalidate_all_line_rects();
}

void Display::scroll(int vertical, int horizontal) {
    set_scroll_offset(display_to_absolute_position({ vertical, horizontal }));
}

RenderedLine& Display::rendered_line_at_index(int index) {
    assert(index < m_rendered_lines.size());

    auto& line = m_rendered_lines[index];
    if (!line.is_up_to_date()) {
        m_rendered_lines[index] = compose_line(document()->line_at_index(index));
    }
    return line;
}

void Display::invalidate_all_lines() {
    m_rendered_lines.clear();
    if (auto doc = document()) {
        m_rendered_lines.resize(doc->num_lines());
    }
    invalidate_all_line_rects();
}

void Display::invalidate_line(int line_index) {
    m_rendered_lines[line_index].invalidate();
    invalidate_all_line_rects();
}

void Display::toggle_show_line_numbers() {
    set_show_line_numbers(!m_show_line_numbers);
}

void Display::set_preview_auto_complete(bool b) {
    if (m_preview_auto_complete == b) {
        return;
    }

    m_preview_auto_complete = b;
    if (document()) {
        invalidate_line(cursors().main_cursor().line_index());
    }
}

void Display::set_show_line_numbers(bool b) {
    if (m_show_line_numbers == b) {
        return;
    }

    m_show_line_numbers = b;
    did_set_show_line_numbers();
}

void Display::set_word_wrap_enabled(bool b) {
    if (m_word_wrap_enabled == b) {
        return;
    }

    m_word_wrap_enabled = b;
    if (document()) {
        invalidate_all_lines();
        for (auto& cursor : cursors()) {
            cursor.compute_max_col(*this);
        }
        set_scroll_offset({ scroll_offset().line_index(), 0, 0 });
        document()->scroll_cursor_into_view(*this, cursors().main_cursor());
    }
}

void Display::toggle_word_wrap_enabled() {
    set_word_wrap_enabled(!m_word_wrap_enabled);
}

void Display::replace_next_search_match(const String& replacement) {
    document()->start_input(*this, true);
    document()->replace_next_search_match(*this, replacement);
    document()->finish_input(*this, true);
}

void Display::move_cursor_to_next_search_match() {
    if (!document() || m_search_results.empty()) {
        return;
    }

    document()->start_input(*this, true);

    cursors().remove_secondary_cursors();
    auto& cursor = cursors().main_cursor();

    if (m_search_result_index >= m_search_results.size()) {
        m_search_result_index = 0;
        document()->move_cursor_to_document_start(*this, cursor);
    }

    while (m_search_results.range(m_search_result_index).ends_before(cursor.index())) {
        m_search_result_index++;
        if (m_search_result_index == m_search_results.size()) {
            m_search_result_index = 0;
            document()->move_cursor_to_document_start(*this, cursor);
        }
    }

    document()->move_cursor_to(*this, cursor, m_search_results.range(m_search_result_index).start());
    document()->move_cursor_to(*this, cursor, m_search_results.range(m_search_result_index).end(), MovementMode::Select);
    m_search_result_index++;
    document()->finish_input(*this, true);
}

void Display::select_next_word_at_cursor() {
    if (!document()) {
        return;
    }

    auto& main_cursor = cursors().main_cursor();
    if (main_cursor.selection().empty()) {
        document()->select_word_at_cursor(*this, main_cursor);
        if (main_cursor.selection().empty()) {
            return;
        }

        auto search_text = document()->selection_text(main_cursor);
        set_search_text(move(search_text));

        // Set m_search_result_index to point just past the current cursor.
        while (m_search_result_index < m_search_results.size() &&
               m_search_results.range(m_search_result_index).ends_before(main_cursor.index())) {
            m_search_result_index++;
        }
        m_search_result_index %= m_search_results.size();
        return;
    } else {
        set_search_text(document()->selection_text(main_cursor));
    }

    auto& result = m_search_results.range(m_search_result_index);
    auto* added_cursor = cursors().add_cursor_at(*document(), result.end(), result.start());
    if (added_cursor) {
        document()->scroll_cursor_into_view(*this, *added_cursor);
    }

    ++m_search_result_index;
    m_search_result_index %= m_search_results.size();
}

void Display::update_search_results() {
    if (!document()) {
        return;
    }

    invalidate_metadata();

    m_search_results.clear();
    if (m_search_text.empty()) {
        return;
    }

    for (int i = 0; i < document()->num_lines(); i++) {
        document()->line_at_index(i).search(*document(), i, m_search_text, m_search_results);
    }

    m_search_result_index = 0;
    while (m_search_result_index < m_search_results.size() &&
           cursors().main_cursor().index() < m_search_results.range(m_search_result_index).start()) {
        m_search_result_index++;
    }
}

void Display::clear_search() {
    if (!document()) {
        return;
    }

    invalidate_metadata();
    m_search_result_index = 0;
    m_search_results.clear();

    m_previous_search_text = move(m_search_text);
}

void Display::set_search_text(String text) {
    if (text == m_search_text) {
        return;
    }

    m_search_text = move(text);
    update_search_results();
}

void Display::install_document_listeners(Document& new_document) {
    new_document.on<DeleteLines>(this_widget(), [this](const DeleteLines& event) {
        m_rendered_lines.remove_count(event.line_index(), event.line_count());
        invalidate_all_line_rects();
        clamp_scroll_offset();
    });

    new_document.on<AddLines>(this_widget(), [this](const AddLines& event) {
        auto new_rendered_lines = Vector<RenderedLine> {};
        new_rendered_lines.resize(event.line_count());

        m_rendered_lines.insert(move(new_rendered_lines), event.line_index());
        invalidate_all_line_rects();
    });

    new_document.on<SplitLines>(this_widget(), [this](const SplitLines& event) {
        m_rendered_lines.insert(RenderedLine {}, event.line_index() + 1);
        invalidate_line(event.line_index());
        invalidate_all_line_rects();
    });

    new_document.on<MergeLines>(this_widget(), [this](const MergeLines& event) {
        m_rendered_lines.remove(event.second_line_index());
        invalidate_line(event.first_line_index());
        invalidate_all_line_rects();
        clamp_scroll_offset();
    });

    new_document.on<AddToLine>(this_widget(), [this](const AddToLine& event) {
        invalidate_line(event.line_index());
    });

    new_document.on<DeleteFromLine>(this_widget(), [this](const DeleteFromLine& event) {
        invalidate_line(event.line_index());
        clamp_scroll_offset();
    });

    new_document.on<MoveLineTo>(this_widget(), [this](const MoveLineTo& event) {
        auto line_min = min(event.line(), event.destination());
        auto line_max = max(event.line(), event.destination());

        if (event.line() > event.destination()) {
            m_rendered_lines.rotate_right(line_min, line_max + 1);
        } else {
            m_rendered_lines.rotate_left(line_min, line_max + 1);
        }

        invalidate_all_line_rects();
        clamp_scroll_offset();
    });

    new_document.on<SyntaxHighlightingChanged>(this_widget(), [this](auto&) {
        invalidate_metadata();
    });

    new_document.on<Change>(this_widget(), [this](auto&) {
        update_search_results();
    });

    cursors().install_document_listeners(new_document);
}

void Display::uninstall_document_listeners(Document& document) {
    document.remove_listener(this_widget());
}

void Display::render_lines() {
    if (!document()) {
        return;
    }

    auto render_position = scroll_offset();
    for (int row_in_display = 0; row_in_display < rows() && render_position.line_index() < document()->num_lines();) {
        auto& line = rendered_line_at_index(render_position.line_index());
        row_in_display += line.render(*this, { render_position.line_index(), 0 }, render_position.relative_col(),
                                      render_position.relative_row(), row_in_display);
        render_position.set_line_index(render_position.line_index() + 1);
        render_position.set_relative_row(0);
    }
}

int Display::absolute_col_offset_of_index(const TextIndex& index) const {
    return rendered_line_at_index(index.line_index()).absolute_col_offset_of_index(index);
}

int Display::rendered_line_count(int line_index) const {
    return rendered_line_at_index(line_index).rendered_line_count();
}

TextIndex Display::prev_index_into_line(const TextIndex& index) const {
    return rendered_line_at_index(index.line_index()).prev_index_into_line(index);
}

TextIndex Display::next_index_into_line(const TextIndex& index) const {
    return rendered_line_at_index(index.line_index()).next_index_into_line(index);
}

TextIndex Display::text_index_at_absolute_position(const AbsolutePosition& position) const {
    return rendered_line_at_index(position.line_index()).index_of_absolute_position(position);
}

TextIndex Display::text_index_at_display_position(const DisplayPosition& position) const {
    return text_index_at_absolute_position(display_to_absolute_position(position));
}

AbsolutePosition Display::display_to_absolute_position(const DisplayPosition& display_position) const {
    auto absolute_position = scroll_offset();

    auto position = display_position;
    while (position.row() < 0) {
        if (absolute_position.line_index() == 0 && absolute_position.relative_row() == 0) {
            break;
        }
        if (absolute_position.relative_row() == 0) {
            absolute_position.set_line_index(absolute_position.line_index() - 1);
            absolute_position.set_relative_row(rendered_line_count(absolute_position.line_index()) - 1);
        } else {
            absolute_position.set_relative_row(absolute_position.relative_row() - 1);
        }
        position.set_row(position.row() + 1);
    }
    while (position.row() > 0) {
        if (absolute_position.line_index() == document()->last_line_index() &&
            absolute_position.relative_row() == rendered_line_count(document()->last_line_index()) - 1) {
            break;
        }
        if (absolute_position.relative_row() == rendered_line_count(absolute_position.line_index()) - 1) {
            absolute_position.set_line_index(absolute_position.line_index() + 1);
            absolute_position.set_relative_row(0);
        } else {
            absolute_position.set_relative_row(absolute_position.relative_row() + 1);
        }
        position.set_row(position.row() - 1);
    }

    absolute_position.set_relative_col(absolute_position.relative_col() + position.col());
    return absolute_position;
}

AbsolutePosition Display::absolute_position_of_index(const TextIndex& index) const {
    auto position = rendered_line_at_index(index.line_index()).relative_position_of_index(index);
    return { index.line_index(), position.row(), position.col() };
}

DisplayPosition Display::absolute_to_display_position(const AbsolutePosition& position) const {
    if (!word_wrap_enabled()) {
        return { position.line_index() - scroll_offset().line_index(), position.relative_col() - scroll_offset().relative_col() };
    }

    int row_offset = 0;
    if (position.line_index() <= scroll_offset().line_index()) {
        for (int line_index = scroll_offset().line_index() - 1; line_index >= position.line_index(); line_index--) {
            row_offset -= rendered_line_count(line_index);
        }
    } else {
        for (int line_index = scroll_offset().line_index(); line_index < position.line_index(); line_index++) {
            row_offset += rendered_line_count(line_index);
        }
    }
    return { row_offset + position.relative_row() - scroll_offset().relative_row(),
             position.relative_col() - scroll_offset().relative_col() };
}

DisplayPosition Display::display_position_of_index(const TextIndex& index) const {
    return absolute_to_display_position(absolute_position_of_index(index));
}

Display::RenderingInfo Display::rendering_info_for_metadata(const CharacterMetadata& metadata) const {
    RenderingInfo info;
    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxComment) {
        info.fg = { VGA_COLOR_DARK_GREY };
    }

    if (metadata.highlighted()) {
        info.fg = { VGA_COLOR_BLACK };
        info.bg = { VGA_COLOR_YELLOW };
    }

    if (metadata.selected()) {
        info.fg = {};
        info.bg = { VGA_COLOR_DARK_GREY };
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxOperator) {
        info.fg = { VGA_COLOR_CYAN };
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxKeyword) {
        info.fg = { VGA_COLOR_MAGENTA };
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxNumber) {
        info.fg = { VGA_COLOR_RED };
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxIdentifier) {
        info.fg = { VGA_COLOR_YELLOW };
        info.bold = true;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxString) {
        info.fg = { VGA_COLOR_GREEN };
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxImportant) {
        info.bold = true;
    }

    if (metadata.highlighted() && metadata.selected()) {
        info.fg = { VGA_COLOR_YELLOW };
        info.bg = { VGA_COLOR_DARK_GREY };
    }

    if (metadata.auto_complete_preview()) {
        info.fg = { VGA_COLOR_DARK_GREY };
    }

    if (metadata.main_cursor()) {
        info.main_cursor = true;
    }

    if (metadata.secondary_cursor()) {
        info.secondary_cursor = true;
    }

    return info;
}

Task<Maybe<String>> Display::prompt(String, String) {
    co_return {};
}

App::ObjectBoundCoroutine Display::do_open_prompt() {
    co_return;
}
}
