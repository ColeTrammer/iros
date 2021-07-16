#pragma once

#include <edit/cursor.h>
#include <edit/forward.h>
#include <liim/maybe.h>
#include <liim/vector.h>

namespace Edit {
enum class AddCursorMode {
    Up,
    Down,
};

class MultiCursor {
public:
    MultiCursor();

    void remove_duplicate_cursors();
    void remove_secondary_cursors();
    Cursor& main_cursor();
    const Cursor& main_cursor() const { return const_cast<MultiCursor&>(*this).main_cursor(); }

    void add_cursor(Document& document, AddCursorMode mode);

    void did_delete_lines(Document& document, int line_index, int line_count);
    void did_add_lines(Document& document, int line_index, int line_count);

    void did_add_to_line(Document& document, int line_index, int index_into_line, int bytes_added);
    void did_delete_from_line(Document& document, int line_index, int index_into_line, int bytes_deleted);

    bool should_show_auto_complete_text_at(const Document& document, const Line& line, int index_into_line) const;
    Maybe<String> preview_auto_complete_text(const Panel& panel) const;

    TextRangeCollection cursor_text_ranges(const Document& document) const;
    TextRangeCollection selections(const Document& document) const;

    int size() const { return m_cursors.size(); }

    Cursor& operator[](int index) { return m_cursors[index]; }
    const Cursor& operator[](int index) const { return m_cursors[index]; }

    auto begin() { return m_cursors.begin(); }
    auto end() { return m_cursors.end(); }

    auto begin() const { return m_cursors.begin(); }
    auto end() const { return m_cursors.end(); }

private:
    Vector<Cursor> m_cursors;
    int m_main_cursor_index { 0 };
};
}
