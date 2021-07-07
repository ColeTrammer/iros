#include <edit/multicursor.h>
#include <edit/text_range_collection.h>

namespace Edit {
MultiCursor::MultiCursor() : m_cursors(Vector<Cursor>::create_from_single_element({})) {}

void MultiCursor::remove_duplicate_cursors() {
    for (int i = m_cursors.size() - 1; i > 0; i--) {
        for (int j = 0; j < i; j++) {
            if (m_cursors[i].index() == m_cursors[j].index()) {
                m_cursors.remove(i);
            }
        }
    }
}

void MultiCursor::remove_secondary_cursors() {
    while (m_cursors.size() > 1) {
        m_cursors.remove_last();
    }
}

Cursor& MultiCursor::main_cursor() {
    return m_cursors.first();
}

Cursor& MultiCursor::add_cursor() {
    m_cursors.add({ m_cursors.last() });
    return m_cursors.last();
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
