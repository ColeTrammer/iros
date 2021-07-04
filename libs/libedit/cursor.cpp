#include <edit/cursor.h>
#include <edit/document.h>
#include <edit/position.h>

namespace Edit {
Line& Cursor::referenced_line() const {
    return m_document.line_at_index(line_index());
}

char Cursor::referenced_character() const {
    return referenced_line().char_at(index_into_line());
}

Position Cursor::relative_position(const Panel& panel) const {
    return referenced_line().relative_position_of_index(m_document, panel, index_into_line());
}

Position Cursor::absolute_position(const Panel& panel) const {
    auto relative_pos = relative_position(panel);
    return { relative_pos.row + line_index(), relative_pos.col };
}

bool Cursor::at_document_end() const {
    auto& last_line = m_document.last_line();
    return &referenced_line() == &last_line && index_into_line() == last_line.length();
}
}
