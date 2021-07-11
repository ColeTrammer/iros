#include <edit/document.h>
#include <edit/multicursor.h>
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

void MultiCursor::add_cursor(Document& document, AddCursorMode mode) {
    switch (mode) {
        case AddCursorMode::Up:
            m_cursors.insert(m_cursors.first(), 0);
            document.move_cursor_up(m_cursors.first());
            m_main_cursor_index++;
            break;
        case AddCursorMode::Down:
            m_cursors.add(m_cursors.last());
            document.move_cursor_down(m_cursors.last());
            break;
    }
}

void MultiCursor::did_delete_lines(int cursor_index, [[maybe_unused]] int line_index, int line_count) {
    for (int i = cursor_index + 1; i < m_cursors.size(); i++) {
        auto& cursor = m_cursors[i];
        cursor.move_up_preserving_selection(line_count);
    }
}

void MultiCursor::did_add_lines(int cursor_index, [[maybe_unused]] int line_index, int line_count) {
    for (int i = cursor_index + 1; i < m_cursors.size(); i++) {
        auto& cursor = m_cursors[i];
        cursor.move_down_preserving_selection(line_count);
    }
}

void MultiCursor::did_add_to_line(int cursor_index, int line_index, [[maybe_unused]] int index_into_line, int bytes_added) {
    for (int i = cursor_index + 1; i < m_cursors.size(); i++) {
        auto& cursor = m_cursors[i];
        if (cursor.line_index() > line_index) {
            break;
        }

        cursor.move_right_preserving_selection(bytes_added);
    }
}

void MultiCursor::did_delete_from_line(int cursor_index, int line_index, [[maybe_unused]] int index_into_line, int bytes_deleted) {
    for (int i = cursor_index + 1; i < m_cursors.size(); i++) {
        auto& cursor = m_cursors[i];
        if (cursor.line_index() > line_index) {
            break;
        }

        cursor.move_left_preserving_selection(bytes_deleted);
    }
}

TextRangeCollection MultiCursor::selections(const Document& document) const {
    auto collection = TextRangeCollection { document };
    for (auto& cursor : m_cursors) {
        collection.add(cursor.selection().text_range());
    }
    collection.sort();
    return collection;
}
}
