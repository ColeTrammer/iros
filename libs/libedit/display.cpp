#include <edit/display.h>
#include <edit/document.h>
#include <edit/rendered_line.h>
#include <eventloop/widget_events.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace Edit {
Display::Display() : m_cursors { *this } {}

void Display::initialize() {
    this_widget().on_unchecked<App::ResizeEvent>({}, [this](auto&) {
        if (word_wrap_enabled()) {
            invalidate_all_lines();
            set_scroll_offset({ scroll_offset().line_index(), 0, 0 });
        }
    });
}

Display::~Display() {
    if (auto* doc = document()) {
        doc->unregister_display(*this, false);
    }
}

void Display::set_document(SharedPtr<Document> document) {
    if (m_document == document) {
        return;
    }

    if (m_document) {
        uninstall_document_listeners(*m_document);
        m_document->unregister_display(*this, true);
        m_search_results.clear();
    }
    m_document = move(document);
    if (m_document) {
        install_document_listeners(*m_document);
        m_document->register_display(*this);
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
    if (scroll_offset().line_index() >= document()->num_lines()) {
        set_scroll_offset({ document()->num_lines() - 1, document()->last_line().rendered_line_count(*document(), *this) - 1,
                            scroll_offset().relative_col() });
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
    set_scroll_offset(document()->display_to_absolute_position(*this, { vertical, horizontal }));
}

RenderedLine& Display::rendered_line_at_index(int index) {
    assert(index < m_rendered_lines.size());
    return m_rendered_lines[index];
}

void Display::invalidate_all_lines() {
    m_rendered_lines.clear();
    if (auto doc = document()) {
        m_rendered_lines.resize(doc->num_lines());
    }
    invalidate_all_line_rects();
}

void Display::invalidate_line(int line_index) {
    auto& info = rendered_line_at_index(line_index);

    invalidate_all_line_rects();

    info.rendered_lines.clear();
    info.position_ranges.clear();
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
            cursor.compute_max_col(*document(), *this);
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
        document()->line_at_index(i).search(*document(), m_search_text, m_search_results);
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

void Display::update_metadata(int line_index) {
    auto& rendered_contents = m_rendered_lines[line_index];

    auto cursor_collection = cursors().cursor_text_ranges();
    auto selection_collection = cursors().selections();
    auto metadata_iterator = Edit::DocumentTextRangeIterator {
        { line_index, 0 }, document()->syntax_highlighting_info(), search_results(), cursor_collection, selection_collection
    };

    for (int row = 0; row < rendered_contents.position_ranges.size(); row++) {
        for (auto& range : rendered_contents.position_ranges[row]) {
            if (range.type == PositionRangeType::Normal) {
                metadata_iterator.advance_to_index_into_line(*document(), range.index_into_line);
                range.metadata = metadata_iterator.peek_metadata();
            }
        }
    }
}

void Display::install_document_listeners(Document& new_document) {
    new_document.on<DeleteLines>(this_widget(), [this](const DeleteLines& event) {
        for (int i = 0; i < event.line_count(); i++) {
            m_rendered_lines.remove(event.line_index());
        }
        invalidate_all_line_rects();
        clamp_scroll_offset();
    });

    new_document.on<AddLines>(this_widget(), [this](const AddLines& event) {
        for (int i = 0; i < event.line_count(); i++) {
            m_rendered_lines.insert({}, event.line_index());
        }
        invalidate_all_line_rects();
    });

    new_document.on<SplitLines>(this_widget(), [this](const SplitLines& event) {
        m_rendered_lines.insert({}, event.line_index() + 1);
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

Display::RenderingInfo Display::rendering_info_for_metadata(const CharacterMetadata& metadata) const {
    RenderingInfo info;
    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxComment) {
        info.fg = VGA_COLOR_DARK_GREY;
    }

    if (metadata.highlighted()) {
        info.fg = VGA_COLOR_BLACK;
        info.bg = VGA_COLOR_YELLOW;
    }

    if (metadata.selected()) {
        info.fg = {};
        info.bg = VGA_COLOR_DARK_GREY;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxOperator) {
        info.fg = VGA_COLOR_CYAN;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxKeyword) {
        info.fg = VGA_COLOR_MAGENTA;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxNumber) {
        info.fg = VGA_COLOR_RED;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxIdentifier) {
        info.fg = VGA_COLOR_YELLOW;
        info.bold = true;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxString) {
        info.fg = VGA_COLOR_GREEN;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxImportant) {
        info.bold = true;
    }

    if (metadata.highlighted() && metadata.selected()) {
        info.fg = VGA_COLOR_YELLOW;
        info.bg = VGA_COLOR_DARK_GREY;
    }

    if (metadata.auto_complete_preview()) {
        info.fg = VGA_COLOR_DARK_GREY;
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
