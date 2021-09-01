#pragma once

#include <edit/cursor.h>
#include <edit/forward.h>
#include <liim/forward.h>
#include <liim/maybe.h>
#include <liim/vector.h>

namespace Edit {
enum class AddCursorMode {
    Up,
    Down,
};

class MultiCursor {
public:
    using Snapshot = Vector<Cursor>;

    explicit MultiCursor(Display& display);

    void remove_duplicate_cursors();
    void remove_secondary_cursors();
    Cursor& main_cursor();
    const Cursor& main_cursor() const { return const_cast<MultiCursor&>(*this).main_cursor(); }

    void add_cursor(Document& document, AddCursorMode mode);
    void add_cursor_at(Document& document, const TextIndex& index, const Selection& selection = {});

    bool should_show_auto_complete_text_at(const Document& document, const Line& line, int index_into_line) const;
    Maybe<String> preview_auto_complete_text() const;

    TextRangeCollection cursor_text_ranges(const Document& document) const;
    TextRangeCollection selections(const Document& document) const;

    int size() const { return m_cursors.size(); }

    Cursor& operator[](int index) { return m_cursors[index]; }
    const Cursor& operator[](int index) const { return m_cursors[index]; }

    auto begin() { return m_cursors.begin(); }
    auto end() { return m_cursors.end(); }

    auto begin() const { return m_cursors.begin(); }
    auto end() const { return m_cursors.end(); }

    void install_document_listeners(Document& document);

    Snapshot snapshot() const;
    void restore(const Snapshot& snapshot);

    void invalidate_based_on_last_snapshot();

    void invalidate_cursor_history();
    void cursor_save();
    void cursor_undo();

private:
    Display& m_display;
    Vector<Cursor> m_cursors;
    Vector<Snapshot> m_history;
    int m_main_cursor_index { 0 };
};
}
