#include <edit/cursor.h>
#include <edit/display.h>
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

RelativePosition Cursor::relative_position(Display& display) const {
    auto absolute_position = this->absolute_position(display);
    return { absolute_position.relative_row(), absolute_position.relative_col() };
}

AbsolutePosition Cursor::absolute_position(Display& display) const {
    return display.absolute_position_of_index(index());
}

void Cursor::compute_max_col(Display& display) {
    m_max_col = absolute_position(display).relative_col();
}

void Cursor::move_preserving_selection(int delta_line_index, int delta_index_into_line) {
    set({ line_index() + delta_line_index, index_into_line() + delta_index_into_line });
    if (m_selection_anchor) {
        set_selection_anchor(
            { selection_anchor().line_index() + delta_line_index, selection_anchor().index_into_line() + delta_index_into_line });
    }
}

TextIndex Cursor::normalized_selection_start() const {
    if (index() <= selection_anchor()) {
        return index();
    }
    return selection_anchor();
}

TextIndex Cursor::normalized_selection_end() const {
    if (index() >= selection_anchor()) {
        return index();
    }
    return selection_anchor();
}

TextRange Cursor::selection() const {
    return { normalized_selection_start(), normalized_selection_end(), CharacterMetadata::Flags::Selected };
}

String Cursor::selection_text(const Document& document) const {
    return document.text_in_range(selection());
}

void Cursor::merge_selections(const Cursor& other) {
    if (this->index() == normalized_selection_start()) {
        set(min(this->normalized_selection_start(), other.normalized_selection_start()));
        set_selection_anchor(max(this->normalized_selection_end(), other.normalized_selection_end()));
    } else {
        set_selection_anchor(min(this->normalized_selection_start(), other.normalized_selection_start()));
        set(max(this->normalized_selection_end(), other.normalized_selection_end()));
    }
}

bool Cursor::at_document_end(const Document& document) const {
    auto& last_line = document.last_line();
    return &referenced_line(document) == &last_line && index_into_line() == last_line.length();
}

bool Cursor::at_line_end(const Document& document) const {
    return index_into_line() == referenced_line(document).length();
}

bool Cursor::at_last_line(const Document& document) const {
    return line_index() == document.last_line_index();
}
}
