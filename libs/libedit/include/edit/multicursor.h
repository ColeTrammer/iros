#pragma once

#include <edit/cursor.h>
#include <edit/forward.h>
#include <liim/vector.h>

namespace Edit {
class MultiCursor {
public:
    MultiCursor();

    void remove_duplicate_cursors();
    void remove_secondary_cursors();
    Cursor& main_cursor();
    const Cursor& main_cursor() const { return const_cast<MultiCursor&>(*this).main_cursor(); }

    Cursor& add_cursor();

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
};
}
