#pragma once

#include <edit/forward.h>
#include <liim/string.h>

namespace Edit {
constexpr int tab_width = 4;

class Line {
public:
    explicit Line(String contents);
    ~Line();

    void overwrite(Document& document, Line&& line);

    int length() const { return m_contents.size(); }
    bool empty() const { return m_contents.size() == 0; }

    const String& contents() const { return m_contents; }

    void insert_char_at(Document& document, int position, char c);
    void remove_char_at(Document& document, int position);

    void combine_line(Document& document, Line& line);

    LineSplitResult split_at(int position);

    Position relative_position_of_index(const Document& document, Panel& panel, int index) const;
    int absoulte_col_offset_of_index(const Document& document, Panel& panel, int index) const;
    int index_of_relative_position(const Document& document, Panel& panel, const Position& position) const;
    int max_col_in_relative_row(const Document& document, Panel& panel, int relative_row) const;
    int absolute_row_position(const Document& document, Panel& panel) const;
    int rendered_line_count(const Document&, Panel&) const;

    char char_at(int index) const { return contents()[index]; }

    void search(const Document& document, const String& text, TextRangeCollection& results) const;

    int render(const Document& document, Panel& panel, DocumentTextRangeIterator& metadata_iterator, int col_offset, int relative_row_start,
               int row_in_panel) const;

    void invalidate_rendered_contents(const Document& document, Panel& panel) const;

private:
    const RenderedLine& compute_rendered_contents(const Document& document, Panel& panel) const;

    enum class RangeFor {
        Text,
        Cursor,
    };

    const PositionRange* range_for_relative_position(const Document& document, Panel& panel, const Position& position) const;
    const PositionRange* range_for_index_into_line(const Document& documnet, Panel& panel, int index_into_line, RangeFor mode) const;

    String m_contents;
};

struct LineSplitResult {
    Line first;
    Line second;
};
}
