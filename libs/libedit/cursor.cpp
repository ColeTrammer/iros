#include <edit/cursor.h>
#include <edit/document.h>

namespace Edit {
Line& Cursor::referenced_line() const {
    return m_document.line_at_index(line_index());
}

char Cursor::referenced_character() const {
    return referenced_line().char_at(index_into_line());
}

int Cursor::row_position(const Panel&) const {
    return line_index();
}

int Cursor::col_position(const Panel& panel) const {
    return referenced_line().col_position_of_index(m_document, panel, index_into_line());
}

bool Cursor::at_document_end() const {
    auto& last_line = m_document.last_line();
    return &referenced_line() == &last_line && index_into_line() == last_line.length();
}
}
