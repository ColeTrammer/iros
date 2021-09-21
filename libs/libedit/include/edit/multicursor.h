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
    struct Snapshot {
        Vector<Cursor> cursors;
        int main_cursor_index { 0 };

        Cursor& operator[](int index) { return cursors[index]; }
        const Cursor& operator[](int index) const { return cursors[index]; }
        auto begin() const { return cursors.begin(); }
        auto end() const { return cursors.end(); }

        auto empty() const { return cursors.empty(); }
        auto size() const { return cursors.size(); }

        bool operator==(const Snapshot& other) const {
            return this->main_cursor_index == other.main_cursor_index && this->cursors == other.cursors;
        }
    };

    explicit MultiCursor(Display& display);

    void remove_duplicate_cursors();
    void remove_secondary_cursors();
    Cursor& main_cursor();
    const Cursor& main_cursor() const { return const_cast<MultiCursor&>(*this).main_cursor(); }

    void add_cursor(Document& document, AddCursorMode mode);
    void add_cursor_at(Document& document, const TextIndex& index, const TextIndex& selection_start);

    bool should_show_auto_complete_text_at(const Document& document, const Line& line, int index_into_line) const;
    Maybe<String> preview_auto_complete_text() const;

    TextRangeCollection cursor_text_ranges() const;
    TextRangeCollection selections() const;

    int size() const { return m_cursors.size(); }

    Cursor& operator[](int index) { return m_cursors[index]; }
    const Cursor& operator[](int index) const { return m_cursors[index]; }

    auto begin() { return m_cursors.begin(); }
    auto end() { return m_cursors.end(); }

    auto begin() const { return m_cursors.begin(); }
    auto end() const { return m_cursors.end(); }

    void install_document_listeners(Document& document);

    Snapshot snapshot() const;
    void restore(Document& document, const Snapshot& snapshot);

    void invalidate_based_on_last_snapshot(Document& document);

    void invalidate_cursor_history();
    void cursor_save();
    void cursor_undo(Document& document);

private:
    Display& m_display;
    Vector<Cursor> m_cursors;
    Vector<Snapshot> m_history;
    int m_main_cursor_index { 0 };
};
}
