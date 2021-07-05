#pragma once

#include <edit/character_metadata.h>
#include <edit/forward.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace Edit {
constexpr int tab_width = 4;

class Line {
public:
    explicit Line(String contents);
    ~Line();

    int length() const { return m_contents.size(); }
    bool empty() const { return m_contents.size() == 0; }

    const String& contents() const { return m_contents; }

    void insert_char_at(int position, char c);
    void remove_char_at(int position);

    void combine_line(Line& line);

    LineSplitResult split_at(int position);

    Position relative_position_of_index(const Document& document, const Panel& panel, int index) const;
    int rendered_string_offset_of_index(const Document& document, const Panel& panel, int index) const;
    int index_of_relative_position(const Document& document, const Panel& panel, const Position& position) const;
    int max_col_in_relative_row(const Document& document, const Panel& panel, int relative_row) const;
    int absolute_row_position(const Document& document, const Panel& panel) const;
    int rendered_line_count(const Document&, const Panel&) const;

    char char_at(int index) const { return contents()[index]; }

    int search(const String& text);

    int render(const Document& document, Panel& panel, DocumentTextRangeIterator& metadata_iterator, int col_offset, int relative_row_start,
               int row_in_panel) const;

    void invalidate_rendered_contents() {
        m_rendered_contents_valid = false;
        m_rendered_line_count_valid = false;
    }

private:
    void compute_rendered_contents(const Document& document, const Panel& panel) const;

    struct IndexRenderedSpan {
        int rendered_start { 0 };
        int rendered_end { 0 };
    };

    String m_contents;
    mutable String m_rendered_contents;
    mutable Vector<IndexRenderedSpan> m_rendered_spans;
    mutable bool m_rendered_contents_valid { false };
    mutable int m_rendered_line_count { 0 };
    mutable bool m_rendered_line_count_valid { false };
};

struct LineSplitResult {
    Line first;
    Line second;
};
}
