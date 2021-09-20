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
}
