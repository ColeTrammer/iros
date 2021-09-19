#include <edit/cursor.h>
#include <edit/document.h>

namespace Edit {
Line& Cursor::referenced_line(Document& document) const {
    return document.line_at_index(line_index());
}

const Line& Cursor::referenced_line(const Document& document) const {
    return document.line_at_index(line_index());
}

char Cursor::referenced_character(const Document& document) const {
    return referenced_line(document).char_at(index_into_line());
}

RelativePosition Cursor::relative_position(const Document& document, Display& display) const {
    return referenced_line(document).relative_position_of_index(document, display, index_into_line());
}

AbsolutePosition Cursor::absolute_position(const Document& document, Display& display) const {
    auto relative_pos = relative_position(document, display);
    return document.relative_to_absolute_position(display, referenced_line(document), relative_pos);
}

void Cursor::compute_max_col(const Document& document, Display& display) {
    m_max_col = referenced_line(document).relative_position_of_index(document, display, index_into_line()).col();
}

void Cursor::move_preserving_selection(int delta_line_index, int delta_index_into_line) {
    set({ line_index() + delta_line_index, index_into_line() + delta_index_into_line });
    if (!m_selection.empty()) {
        m_selection.set(
            { m_selection.start().line_index() + delta_line_index, m_selection.start().index_into_line() + delta_index_into_line },
            { m_selection.end().line_index() + delta_line_index, m_selection.end().index_into_line() + delta_index_into_line });
    } else {
        m_selection.set(index(), index());
    }
}

bool Cursor::at_document_end(const Document& document) const {
    auto& last_line = document.last_line();
    return &referenced_line(document) == &last_line && index_into_line() == last_line.length();
}
}
