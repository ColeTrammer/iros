#pragma once

#include <assert.h>
#include <edit/forward.h>
#include <edit/selection.h>
#include <edit/text_index.h>
#include <stddef.h>

namespace Edit {
class Cursor {
public:
    Line& referenced_line(Document& document) const;
    const Line& referenced_line(const Document& document) const;
    char referenced_character(const Document& document) const;

    int line_index() const { return m_index.line_index(); }
    int index_into_line() const { return m_index.index_into_line(); }
    const TextIndex& index() const { return m_index; }

    void set_line_index(int line_index) { set({ line_index, index_into_line() }); }
    void set_index_into_line(int index_into_line) { set({ line_index(), index_into_line }); }
    void set(const TextIndex& index) { m_index = index; }

    void move_up_preserving_selection(int count) { move_preserving_selection(-count, 0); }
    void move_down_preserving_selection(int count) { move_preserving_selection(count, 0); }

    void move_left_preserving_selection(int count) { move_preserving_selection(0, -count); }
    void move_right_preserving_selection(int count) { move_preserving_selection(0, count); }

    void move_preserving_selection(int delta_line_index, int delta_index_into_line);

    Position relative_position(const Document& document, Display& display) const;
    Position absolute_position(const Document& document, Display& display) const;

    bool at_document_start(const Document&) const { return m_index == TextIndex { 0, 0 }; }
    bool at_document_end(const Document& document) const;

    Selection& selection() { return m_selection; }
    const Selection& selection() const { return m_selection; }

    int max_col() const { return m_max_col; }
    void compute_max_col(const Document& document, Display& display);

private:
    TextIndex m_index;
    Selection m_selection;
    int m_max_col { 0 };
};
};

namespace LIIM::Format {
template<>
struct Formatter<Edit::Cursor> : public Formatter<String> {
    void format(const Edit::Cursor& cursor, FormatContext& context) {
        return Formatter<String>::format(::format("Cursor <line_index={} index_into_line={} max_col={} selection={}>", cursor.line_index(),
                                                  cursor.index_into_line(), cursor.max_col(), cursor.selection()),
                                         context);
    }
};
}
