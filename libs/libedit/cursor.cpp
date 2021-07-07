#include <edit/cursor.h>
#include <edit/document.h>
#include <edit/position.h>

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

Position Cursor::relative_position(const Document& document, const Panel& panel) const {
    return referenced_line(document).relative_position_of_index(document, panel, index_into_line());
}

Position Cursor::absolute_position(const Document& document, const Panel& panel) const {
    auto relative_pos = relative_position(document, panel);
    return document.relative_to_absolute_position(panel, referenced_line(document), relative_pos);
}

bool Cursor::at_document_end(const Document& document) const {
    auto& last_line = document.last_line();
    return &referenced_line(document) == &last_line && index_into_line() == last_line.length();
}
}
