#include <edit/document.h>
#include <edit/multicursor.h>
#include <edit/panel.h>
#include <edit/position.h>
#include <edit/text_range_collection.h>

namespace Edit {
MultiCursor::MultiCursor() : m_cursors(Vector<Cursor>::create_from_single_element({})) {}

void MultiCursor::remove_duplicate_cursors() {
    for (int i = 0; i < m_cursors.size(); i++) {
        auto& current = m_cursors[i];

        // NOTE: for Selection::overlaps() to behave correctly, empty selections must be "positioned" at the cursor.
        if (current.selection().empty()) {
            current.selection().begin(current.index());
        }
        while (i + 1 < m_cursors.size()) {
            auto& next_cursor = m_cursors[i + 1];
            if (next_cursor.selection().empty()) {
                next_cursor.selection().begin(next_cursor.index());
            }

            if (current.selection().overlaps(next_cursor.selection())) {
                current.selection().merge(m_cursors[i + 1].selection());
                m_cursors.remove(i + 1);
                if (m_main_cursor_index >= i + 1) {
                    m_main_cursor_index--;
                }
                continue;
            }
            break;
        }
    }
}

void MultiCursor::remove_secondary_cursors() {
    auto main_cursor_save = main_cursor();
    m_cursors.clear();
    m_cursors.add(main_cursor_save);
    m_main_cursor_index = 0;
}

Cursor& MultiCursor::main_cursor() {
    return m_cursors[m_main_cursor_index];
}

void MultiCursor::add_cursor(Document& document, Panel& panel, AddCursorMode mode) {
    switch (mode) {
        case AddCursorMode::Up:
            m_cursors.insert(m_cursors.first(), 0);
            document.move_cursor_up(panel, m_cursors.first());
            m_main_cursor_index++;
            break;
        case AddCursorMode::Down:
            m_cursors.add(m_cursors.last());
            document.move_cursor_down(panel, m_cursors.last());
            break;
    }
}

void MultiCursor::did_delete_lines(Document& document, Panel& panel, int line_index, int line_count) {
    for (auto& cursor : m_cursors) {
        if (cursor.line_index() < line_index) {
            continue;
        }
        if (cursor.line_index() == line_index) {
            if (line_index == document.num_lines()) {
                cursor.set({ document.num_lines() - 1, document.last_line().length() });
                continue;
            }
            document.clear_selection(cursor);
            document.move_cursor_to(panel, cursor,
                                    document.text_index_at_absolute_position(panel, cursor.absolute_position(document, panel)));
            continue;
        }
        cursor.move_up_preserving_selection(line_count);
    }
}

void MultiCursor::did_add_lines(Document&, Panel&, int line_index, int line_count) {
    for (auto& cursor : m_cursors) {
        if (cursor.line_index() < line_index) {
            continue;
        }
        cursor.move_down_preserving_selection(line_count);
    }
}

void MultiCursor::did_split_line(Document& document, Panel& panel, int line_index, int index_into_line) {
    for (auto& cursor : m_cursors) {
        if (cursor.line_index() != line_index || cursor.index_into_line() < index_into_line) {
            continue;
        }
        cursor.set({ line_index + 1, cursor.index_into_line() - index_into_line });
        cursor.compute_max_col(document, panel);
    }
}

void MultiCursor::did_merge_lines(Document& document, Panel& panel, int first_line_index, int first_line_length, int second_line_index) {
    for (auto& cursor : m_cursors) {
        if (cursor.line_index() != second_line_index) {
            continue;
        }
        cursor.set({ first_line_index, first_line_length + cursor.index_into_line() });
        cursor.compute_max_col(document, panel);
    }
}

void MultiCursor::did_add_to_line(Document& document, Panel& panel, int line_index, int index_into_line, int bytes_added) {
    for (auto& cursor : m_cursors) {
        if (cursor.line_index() != line_index) {
            continue;
        }
        if (cursor.index_into_line() < index_into_line) {
            continue;
        }
        cursor.move_right_preserving_selection(bytes_added);
        cursor.compute_max_col(document, panel);
    }
}

void MultiCursor::did_delete_from_line(Document& document, Panel& panel, int line_index, int index_into_line, int bytes_deleted) {
    for (auto& cursor : m_cursors) {
        if (cursor.line_index() != line_index) {
            continue;
        }
        if (cursor.index_into_line() <= index_into_line) {
            continue;
        }
        cursor.move_left_preserving_selection(bytes_deleted);
        cursor.compute_max_col(document, panel);
    }
}

bool MultiCursor::should_show_auto_complete_text_at(const Document& document, const Line& line, int index_into_line) const {
    return document.preview_auto_complete() && &main_cursor().referenced_line(document) == &line &&
           main_cursor().index_into_line() == index_into_line;
}

Maybe<String> MultiCursor::preview_auto_complete_text(const Panel& panel) const {
    auto suggestions = panel.get_suggestions();
    if (suggestions.suggestion_count() != 1) {
        return {};
    }

    auto& text = suggestions.suggestion_list().first();
    return text.substring(suggestions.suggestion_offset());
}

TextRangeCollection MultiCursor::cursor_text_ranges(const Document& document) const {
    auto collection = TextRangeCollection { document };
    for (auto& cursor : m_cursors) {
        auto start = cursor.index();
        auto end = TextIndex { cursor.line_index(), cursor.index_into_line() + 1 };
        auto metadata = CharacterMetadata { &cursor == &main_cursor() ? CharacterMetadata::Flags::MainCursor
                                                                      : CharacterMetadata::Flags::SecondaryCursor };
        collection.add(TextRange { start, end, metadata });
    }
    return collection;
}

TextRangeCollection MultiCursor::selections(const Document& document) const {
    auto collection = TextRangeCollection { document };
    for (auto& cursor : m_cursors) {
        collection.add(cursor.selection().text_range());
    }
    return collection;
}
}
