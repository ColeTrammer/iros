#pragma once

#include <edit/character_metadata.h>
#include <edit/forward.h>
#include <edit/position.h>
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
    int absoulte_col_offset_of_index(const Document& document, const Panel& panel, int index) const;
    int index_of_relative_position(const Document& document, const Panel& panel, const Position& position) const;
    int max_col_in_relative_row(const Document& document, const Panel& panel, int relative_row) const;
    int absolute_row_position(const Document& document, const Panel& panel) const;
    int rendered_line_count(const Document&, const Panel&) const;

    char char_at(int index) const { return contents()[index]; }

    void search(const Document& document, const String& text, TextRangeCollection& results);

    int render(const Document& document, Panel& panel, DocumentTextRangeIterator& metadata_iterator, int col_offset, int relative_row_start,
               int row_in_panel) const;

    void invalidate_rendered_contents() {
        m_rendered_lines.clear();
        m_position_ranges.clear();
    }

private:
    void compute_rendered_contents(const Document& document, const Panel& panel) const;

    struct PositionRange {
        Position start;
        Position end;
    };

    String m_contents;
    mutable Vector<String> m_rendered_lines;
    mutable Vector<PositionRange> m_position_ranges;
};

struct LineSplitResult {
    Line first;
    Line second;
};
}
